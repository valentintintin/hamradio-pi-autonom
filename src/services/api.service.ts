import * as express from 'express';
import { Request } from 'express';
import { LogService } from './log.service';
import { SensorsService } from './sensors.service';
import { SstvService } from './sstv.service';
import { ConfigInterface } from '../config/config.interface';
import { WebcamService } from './webcam.service';
import { AprsService } from './aprs.service';
import { VoiceService } from './voice.service';
import { GpioEnum, GpioService } from './gpio.service';
import { catchError } from 'rxjs/operators';
import { of } from 'rxjs';
import { MpptchgService } from './mpptchg.service';
import { RadioService } from './radio.service';

export class ApiService {

    private readonly app: express.Application = express();

    constructor(config: ConfigInterface) {
        this.app.use(express.json());

        this.app.use((req, res, next) => {
            LogService.log('api', 'Request start', req.method, req.path);

            res.header('Access-Control-Allow-Origin', '*');
            res.header('Access-Control-Allow-Headers', 'Origin, X-Requested-With, Content-Type, Accept');

            res.on('finish', () => {
                LogService.log('api', 'Request end', req.method, req.path, res.statusCode);
            });

            next();
        });

        this.app.options('*', (req, res) => {
            res.header('Access-Control-Allow-Methods', 'GET, PATCH, PUT, POST, DELETE, OPTIONS');
            res.send();
        });

        this.app.get('/logs', (req, res) => {
        });

        this.app.post('/gpio/:pin/:value', (req: Request, res) => {
            const pin = GpioEnum[req.params['pin']];
            const value = req.params['value'] === '1';

            if (pin === GpioEnum.RelayRadio) {
                RadioService.keepOn = value;
                LogService.log('radio', 'State keepOn set by API', RadioService.keepOn);
            }

            GpioService.set(pin, value).pipe(
                catchError(e => {
                    res.json(e);
                    return of(null);
                })
            ).subscribe(_ => res.send(true));
        });

        if (config.sensors && config.sensors.enable) {
            this.app.get('/sensors', (req, res) => {
                SensorsService.getAllAndSave(config.sensors).subscribe(datas => res.send(datas));
            });
        }

        if (config.mpptChd && config.mpptChd.enable) {
            this.app.post('/shutdown/{timestamp}', (req, res) => {
                const restartDate = new Date(+req.params['timestamp'] * 1000);
                MpptchgService.shutdownAndWakeUpAtDate(restartDate).pipe(
                    catchError(e => {
                        res.json(e);
                        return of(null);
                    })
                ).subscribe(wakeupDate => res.send(wakeupDate));
            });

            this.app.post('/watchdog/stop', (req, res) => {
                MpptchgService.stopWatchdog().pipe(
                    catchError(e => {
                        res.json(e);
                        return of(null);
                    })
                ).subscribe(wakeupDate => res.send(wakeupDate));
            });

            this.app.post('/watchdog/start', (req, res) => {
                MpptchgService.startWatchdog().pipe(
                    catchError(e => {
                        res.json(e);
                        return of(null);
                    })
                ).subscribe(wakeupDate => res.send(true));
            });
        }

        if (config.sstv && config.sstv.enable) {
            this.app.post('/sstv', (req, res) => {
                SstvService.sendImage(config.sstv).pipe(
                    catchError(e => {
                        res.json(e);
                        return of(null);
                    })
                ).subscribe(_ => res.send(true));
            });
        }

        if (config.webcam && config.webcam.enable) {
            this.app.post('/webcam', (req, res) => {
                WebcamService.captureAndSend(config.webcam, config.sftp).pipe(
                    catchError(e => {
                        res.json(e);
                        return of(null);
                    })
                ).subscribe(filePath => res.sendFile(filePath));
            });
        }

        if (config.voice && config.voice.enable) {
            this.app.post('/voice', (req, res) => {
                VoiceService.sendVoice(config.voice.sentence).pipe(
                    catchError(e => {
                        res.json(e);
                        return of(null);
                    })
                ).subscribe(_ => res.send(true));
            });
        }

        if (config.aprs && config.aprs.enable) {
            this.app.post('/aprs/beacon', (req, res) => {
                AprsService.sendAprsBeacon(config.aprs).pipe(
                    catchError(e => {
                        res.json(e);
                        return of(null);
                    })
                ).subscribe(_ => res.send(true));
            });

            this.app.post('/aprs/telemetry', (req, res) => {
                AprsService.sendAprsTelemetry(config.aprs, config.sensors).pipe(
                    catchError(e => {
                        res.json(e);
                        return of(null);
                    })
                ).subscribe(_ => res.send(true));
            });
        }

        const port = config.api.port ?? 3000;
        this.app.listen(port, () => {
            LogService.log('api', 'Started', port);
        });
    }
}
