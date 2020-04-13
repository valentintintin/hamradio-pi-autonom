import { Observable, Observer } from 'rxjs';
import { LogService } from './log.service';
import { catchError, delay, switchMap } from 'rxjs/operators';
import { RadioService } from './radio.service';
import { SstvConfigInterface } from '../config/sstv-config.interface';
import * as Jimp from 'jimp'
import { WebcamService } from './webcam.service';
import { assetsFolder } from '../index';
import ChildProcess = require('child_process');
import PlaySound = require('play-sound');

export class SstvService {

    private static readonly player = PlaySound();
    private static readonly tmpImage = '/tmp/sstv.jpg';

    public static sendImage(config: SstvConfigInterface, keepRadioOn: boolean = false): Observable<void> {
        LogService.log('sstv', 'Start sending image');

        return new Observable<void>((observer: Observer<void>) => {
            LogService.log('sstv', 'Get image', WebcamService.lastPhotoPath);
            // @ts-ignore
            Jimp.read(WebcamService.lastPhotoPath ?? assetsFolder + '/test.jpg')
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
            catchError(err => {
                LogService.log('sstv', 'Send image KO', err);
                return RadioService.pttOff(!keepRadioOn);
            }),
        );
    }
}
