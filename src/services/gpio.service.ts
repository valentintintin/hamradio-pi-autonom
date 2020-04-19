import { Observable, Observer, of } from 'rxjs';
import { switchMap } from 'rxjs/operators';
import { LogService } from './log.service';

const gpio = require('gpio');

export class GpioService {

    public static USE_FAKE = false;

    private static gpios: any[] = [];

    public static set(pin: number, value: boolean): Observable<void> {
        LogService.log('gpio', 'Set', GpioEnum[pin] + `(${pin})`, value);

        if (GpioService.USE_FAKE || pin === GpioEnum.RelayBorneWifi) {
            return of(null);
        }

        return GpioService.init(pin, gpio.DIRECTION.OUT).pipe(
            switchMap((gpio: any) => {
                return new Observable<void>((observer: Observer<void>) => {
                    gpio.set(value, _ => {
                        observer.next();
                        observer.complete();
                    });
                });
            })
        );
    }

    private static init(pin: number, direction: string): Observable<any> {
        return new Observable<any>((observer: Observer<any>) => {
            if (!this.gpios[pin]) {
                this.gpios[pin] = gpio.export(pin, {
                    direction: direction,
                    ready: _ => {
                        LogService.log('gpio', 'Export pin ' + pin, direction);
                        observer.next(this.gpios[pin]);
                        observer.complete();
                    }
                });
            } else {
                observer.next(GpioService.gpios[pin]);
                observer.complete();
            }
        });
    }
}

export enum GpioEnum {
    RelayRadio = 0,
    RelayBorneWifi = 1,
    PTT = 2
}
