import { Observable, of } from 'rxjs';
import { LogService } from './log.service';
import { catchError, delay, map, switchMap } from 'rxjs/operators';
import { GpioEnum, GpioService } from './gpio.service';
import { ToneService } from './tone.service';
import { AudioService } from './audio.service';

export class RadioService {

    public static keepOn: boolean = false;

    public static switchOn(): Observable<void> {
        LogService.log('radio', 'Relay', true);

        return GpioService.set(GpioEnum.RelayRadio, true).pipe(
            delay(500)
        );
    }

    public static switchOff(): Observable<void> {
        LogService.log('radio', 'Relay', false);
        if (RadioService.keepOn) {
            return of(null);
        }
        return GpioService.set(GpioEnum.RelayRadio, false);
    }

    public static pttOn(): Observable<void> {
        LogService.log('radio', 'PTT', true);

        return RadioService.switchOn().pipe(
            switchMap(_ => GpioService.set(GpioEnum.PTT, true))
        );
    }

    public static pttOff(shutdownRadio: boolean = true): Observable<void> {
        LogService.log('radio', 'PTT', false);

        return GpioService.set(GpioEnum.PTT, false).pipe(
            switchMap(_ => shutdownRadio ? RadioService.switchOff() : of(null))
        );
    }

    public static listenAndRepeat(seconds: number = 10, shutdownRadio: boolean = true): Observable<void> {
        LogService.log('radio', 'Start listening to repeat', seconds);

        return ToneService.sendOk(false, true).pipe(
            switchMap(_ => AudioService.record(seconds)),
            switchMap(path => ToneService.sendOk(true, true).pipe(map(_ => path))),
            switchMap(path => AudioService.play(path)),
            switchMap(_ => ToneService.sendError(false, !shutdownRadio)),
            catchError(err => {
                LogService.log('radio', 'Send image KO', err);
                return RadioService.pttOff(shutdownRadio);
            })
        );
    }
}
