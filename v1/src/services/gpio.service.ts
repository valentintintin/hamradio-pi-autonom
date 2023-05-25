import { Observable, Observer, of } from 'rxjs';
import { switchMap } from 'rxjs/operators';
import { LogService } from './log.service';

const gpio = require('wpi-gpio');

export class GpioService {

    public static USE_FAKE = false;

    private static gpios: any[] = [];

    public static set(pin: number, value: boolean): Observable<void> {
        if (pin === GpioEnum.RelayBorneWifi || pin === GpioEnum.RelayRadio) {
            value = !value;
        }

        if (GpioService.USE_FAKE || pin === GpioEnum.RelayBorneWifi) {
            LogService.log('gpio', 'Set fake', GpioEnum[pin] + `(${pin})`, value);
            return of(null);
        }

        return GpioService.initOutput(pin).pipe(
            switchMap(_ => {
                return new Observable<void>((observer: Observer<void>) => {
                    LogService.log('gpio', 'Write', GpioEnum[pin] + `(${pin})`, value);
                    gpio.write(pin, value).then(_ => {
                        observer.next(null);
                        observer.complete();
                    }).catch(e => observer.error(e));
                });
            })
        );
    }

    private static initOutput(pin: number): Observable<void> {
        return new Observable<void>((observer: Observer<void>) => {
            if (!this.gpios[pin]) {
                LogService.log('gpio', 'Set mode OUTPUT', GpioEnum[pin] + `(${pin})`);
                gpio.output(pin).then(_ => {
                    this.gpios[pin] = true;
                    observer.next(null);
                    observer.complete();
                }).catch(e => observer.error(e));
            } else {
                observer.next(null);
                observer.complete();
            }
        });
    }
}

export enum GpioEnum {
    RelayRadio = 0,
    RelayBorneWifi = 2,
    PTT = 3
}
