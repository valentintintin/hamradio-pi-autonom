import { BehaviorSubject, Observable } from 'rxjs';
import { LogService } from './log.service';

const SerialPort = require('serialport')
const Readline = require('@serialport/parser-readline')

export class SerialService {

    public static USE_FAKE = false;

    private static _instance: SerialService;

    private _data$ = new BehaviorSubject<string>('[LIGHT]0[PRESSURE_TEMP]1.23[PRESSURE]2[TEMP]3.45[HUMIDITY]4.56');
    private port;

    public static get instance(): SerialService {
        if (!this._instance) {
            this._instance = new SerialService();
        }
        return this._instance;
    }

    constructor() {
        const parser = new Readline()
        parser.on('data', line => this._data$.next(line));

        if (!SerialService.USE_FAKE) {
            this.port = new SerialPort('/dev/ttyUSB0', { baudRate: 115200, }, error => {
                if (error) {
                    LogService.log('serial', 'Serial KO', error);
                }
            });
            this.port.pipe(parser);
        }
    }

    public get data(): string {
        return this._data$.value;
    }

    public get data$(): Observable<string> {
        return this._data$.asObservable();
    }
}
