import { Observable, Observer, of } from 'rxjs';
import { LogService } from './log.service';
import { catchError, switchMap, tap } from 'rxjs/operators';
import { RadioService } from './radio.service';
import ChildProcess = require('child_process');

export class ToneService {

    public static alreadyInUse: boolean;

    public static send1750(keepPttOn: boolean = false, keepRadioOn: boolean = false): Observable<void> {
        return ToneService.sendTone(1750, 2, keepPttOn, keepRadioOn);
    }

    public static sendOk(keepPttOn: boolean = false, keepRadioOn: boolean = false): Observable<void> {
        return ToneService.sendTones([525, 0, 525], [0.1, 0.1, 0.1], keepPttOn, keepRadioOn);
    }

    public static sendError(keepPttOn: boolean = false, keepRadioOn: boolean = false): Observable<void> {
        return ToneService.sendTones([400, 0, 400, 0, 400, 0, 400], [0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1], keepPttOn, keepRadioOn);
    }

    public static sendTone(freq: number, seconds: number = 2, keepPttOn: boolean = false, keepRadioOn: boolean = false): Observable<void> {
        return ToneService.sendTones([freq], [seconds], keepPttOn, keepRadioOn);
    }

    public static sendTones(freqs: number[], seconds: number[], keepPttOn: boolean = false, keepRadioOn: boolean = false): Observable<void> {
        LogService.log('tone', 'Start sending tones', freqs);

        if (ToneService.alreadyInUse) {
            LogService.log('tone', 'Already in use');
            return of(null);
        }

        ToneService.alreadyInUse = true;

        let todoSubscription = RadioService.pttOn();

        freqs.forEach((value, index) => {
            return todoSubscription = todoSubscription.pipe(
                switchMap(_ => ToneService.playTone(value, seconds[index])),
                catchError(_ => of(null))
            );
        });

        return todoSubscription.pipe(
            switchMap(_ => keepPttOn ? of(null) : RadioService.pttOff(!keepRadioOn)),
            tap(_ => {
                ToneService.alreadyInUse = false;
                LogService.log('tone', 'Send tones  OK');
            }),
            catchError(err => {
                ToneService.alreadyInUse = false;
                LogService.log('tone', 'Send tones KO', err);
                return keepPttOn ? of(null) : RadioService.pttOff(!keepRadioOn);
            }),
        );
    }

    private static playTone(freq: number, seconds: number): Observable<void> {
        return new Observable<void>((observer: Observer<void>) => {
            LogService.log('tone', 'Playing tone', freq, seconds);
            try {
                ChildProcess.execSync(`play -n synth ${seconds} sine ${freq}`, {
                    encoding: 'utf8',
                    stdio: 'pipe'
                });
                observer.next();
                observer.complete();
            } catch (e) {
                observer.error(e);
            }
        });
    }
}
