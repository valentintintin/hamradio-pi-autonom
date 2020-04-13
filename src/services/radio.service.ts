import { Observable, of } from 'rxjs';
import { LogService } from './log.service';
import { delay, switchMap } from 'rxjs/operators';
import { GpioEnum, GpioService } from './gpio.service';

export class RadioService {

    public static switchOn(): Observable<void> {
        LogService.log('radio', 'Relay', true);
        return GpioService.set(GpioEnum.RelayRadio, true);
    }

    public static switchOff(): Observable<void> {
        LogService.log('radio', 'Relay', false);
        return GpioService.set(GpioEnum.RelayRadio, false);
    }

    public static pttOn(): Observable<void> {
        return RadioService.switchOn().pipe(
            switchMap(_ => {
                LogService.log('radio', 'PTT', true);
                return GpioService.set(GpioEnum.PTT, true);
            }),
            delay(500),
        );
    }

    public static pttOff(shutdownRadio: boolean = true): Observable<void> {
        LogService.log('radio', 'PTT', false);

        return GpioService.set(GpioEnum.PTT, false).pipe(
            switchMap(_ => shutdownRadio ? RadioService.switchOff() : of(null))
        );
    }
}
