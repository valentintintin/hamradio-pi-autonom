import { Observable, Observer, of } from 'rxjs';
import { LogService } from './log.service';
import { catchError, delay, switchMap, tap } from 'rxjs/operators';
import { RadioService } from './radio.service';
import { SstvConfigInterface } from '../config/sstv-config.interface';
import * as Jimp from 'jimp'
import { WebcamService } from './webcam.service';
import { assetsFolder } from '../index';
import ChildProcess = require('child_process');
import PlaySound = require('play-sound');
import fs = require('fs');

export class SstvService {

    private static readonly player = PlaySound();
    private static readonly tmpImage = '/tmp/sstv.jpg';

    public static alreadyInUse: boolean;

    public static sendImage(config: SstvConfigInterface, keepRadioOn: boolean = false): Observable<void> {
        LogService.log('sstv', 'Start sending image');

        if (SstvService.alreadyInUse) {
            LogService.log('sstv', 'Already in use');
            return of(null);
        }

        SstvService.alreadyInUse = true;

        return new Observable<void>((observer: Observer<void>) => {
            const filePath = WebcamService.lastPhotoPath ?? assetsFolder + '/test.jpg';

            LogService.log('sstv', 'Get image', filePath);

            if (!fs.existsSync(filePath)) {
                observer.error(new Error('File does not exist'));
                return;
            }

            // @ts-ignore
            Jimp.read(filePath)
                .then(image => {
                    // @ts-ignore
                    Jimp.loadFont(Jimp.FONT_SANS_16_BLACK).then(font => {
                        image.resize(320, 240)
                            .print(font, 0, 0, config.comment.toUpperCase())
                            .print(font, 0, image.getHeight() - 16, config.comment.toUpperCase())
                            .write(SstvService.tmpImage);
                        observer.next();
                        observer.complete();
                    }).catch(err => observer.error(err));
                })
                .catch(err => observer.error(err));
        }).pipe(
            delay(100),
            switchMap(_ => {
                return new Observable<void>((observer: Observer<void>) => {
                    LogService.log('sstv', 'Generating SSTV');
                    try {
                        ChildProcess.execFileSync(config.pisstvPath + '/pisstv', [
                            '-p' + config.mode,
                            SstvService.tmpImage
                        ], {
                            encoding: 'utf8'
                        });
                        observer.next();
                        observer.complete();
                    } catch (e) {
                        observer.error(e);
                    }
                });
            }),
            switchMap(_ => RadioService.pttOn()),
            switchMap(_ => {
                return new Observable<void>((observer: Observer<void>) => {
                    LogService.log('sstv', 'Sending 1750 Hz');
                    this.player.play(assetsFolder + '/1750.wav', err => {
                        if (err) {
                            observer.error(err);
                        } else {
                            observer.next();
                            observer.complete();
                        }
                    });
                });
            }),
            switchMap(_ => {
                return new Observable<void>((observer: Observer<void>) => {
                    LogService.log('sstv', 'Sending SSTV');
                    this.player.play('/tmp/sstv.jpg.wav', err => {
                        if (err) {
                            LogService.log('sstv', 'Send image KO', err);
                            observer.error(err);
                        } else {
                            observer.next();
                            observer.complete();
                        }
                    });
                });
            }),
            switchMap(_ => RadioService.pttOff(!keepRadioOn)),
            tap(_ => {
                SstvService.alreadyInUse = false;
                LogService.log('sstv', 'Send image OK');
            }),
            catchError(err => {
                SstvService.alreadyInUse = false;
                LogService.log('sstv', 'Send image KO', err);
                return RadioService.pttOff(!keepRadioOn);
            }),
        );
    }
}
