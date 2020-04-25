import { Observable, Observer, of } from 'rxjs';
import { LogService } from './log.service';
import { catchError, switchMap, tap } from 'rxjs/operators';
import { RadioService } from './radio.service';
import { assetsFolder } from '../index';
import PlaySound = require('play-sound');

export class ToneService {

    private static readonly player = PlaySound();
    private static readonly tmpImage = '/tmp/sstv.jpg';

    public static alreadyInUse: boolean;

    public static send1750(keepPttOn: boolean = false, keepRadioOn: boolean = false): Observable<void> {
        LogService.log('tone', 'Start sending tone 1750');

        if (ToneService.alreadyInUse) {
            LogService.log('sstv', 'Already in use');
            return of(null);
        }

        ToneService.alreadyInUse = true;

        return RadioService.pttOn().pipe(
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
            switchMap(_ => keepPttOn ? of(null) : RadioService.pttOff(!keepRadioOn)),
            tap(_ => {
                ToneService.alreadyInUse = false;
                LogService.log('tone', 'Send tone 1750  OK');
            }),
            catchError(err => {
                ToneService.alreadyInUse = false;
                LogService.log('tone', 'Send tone 1750 KO', err);
                return keepPttOn ? of(null) : RadioService.pttOff(!keepRadioOn);
            }),
        );
    }
}
