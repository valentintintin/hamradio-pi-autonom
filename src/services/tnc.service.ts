import { Defs, kissTNCTcp, Packet } from 'ax25';
import { LogService } from './log.service';
import { Observable, Observer, of } from 'rxjs';
import { delay, tap } from 'rxjs/operators';

export class TncService {

    private static _instance: TncService;

    private static readonly SEC_START: number = 2000;
    private static readonly SEC_TRAME: number = 500;

    public static get instance(): TncService {
        if (!this._instance) {
            this._instance = new TncService();
        }
        return this._instance;
    }

    private tnc: kissTNCTcp;
    private connected: boolean;

    private connectTnc(): Observable<void> {
        if (this.tnc && this.connected) {
            return of(null);
        }

        return new Observable<void>((observer: Observer<void>) => {
            LogService.log('tnc', 'Start connection');

            this.tnc = new kissTNCTcp({
                ip: 'localhost',
                port: 8001
            });

            this.tnc.on('error', err => {
                observer.error(err);
            });

            this.tnc.on('opened', () => {
                this.connected = true;
                observer.next(null);
                observer.complete();
            });

            this.tnc.on('frame', frame => {
                const packet = new Packet();
                packet.disassemble(frame);
            });
        });
    }

    private getExplodedCallSign(callSign: string): CallSignExploded {
        const splited = callSign.split('-');

        return {
            callsign: splited[0].trim(),
            ssid: splited.length === 2 && !isNaN(+splited[1]) ? +splited[1] : 0
        };
    }

    private createFrame(from: string, to: string, info: string, path: string = ''): Packet {
        const fromCall = this.getExplodedCallSign(from);
        const toCall = this.getExplodedCallSign(to);

        path = path ?? '';

        const data = {
            sourceCallsign: fromCall.callsign,
            sourceSSID: fromCall.ssid,
            destinationCallsign: toCall.callsign,
            destinationSSID: toCall.ssid,
            repeaterPath: path.split(',').map(r => this.getExplodedCallSign(r)),
            type: Defs.U_FRAME_UI,
            infoString: info
        };

        try {
            return new Packet(data);
        } catch (e) {
            LogService.log('tnc', 'Packet creation error', e, data);
            throw e;
        }
    }

    public send(from: string, to: string, info: string, path: string = ''): Observable<void> {
        return this.sendMultiples(from, to, [info], path);
    }

    public sendMultiples(from: string, to: string, infos: string[], path: string = ''): Observable<void> {
        return this.connectTnc().pipe(
            tap(_ => {
                LogService.log('tnc', 'Start sending', from, to, infos, path);
                infos.forEach(info => this.tnc.send(this.createFrame(from, to, info, path).assemble()));
            }),
            delay(TncService.SEC_START + TncService.SEC_TRAME * infos.length),
            tap(_ => LogService.log('tnc', 'Send OK'))
        );
    }
}

interface CallSignExploded {
    callsign: string;
    ssid: number;
}
