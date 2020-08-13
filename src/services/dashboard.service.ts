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
import { catchError, map, switchMap } from 'rxjs/operators';
import { forkJoin, of } from 'rxjs';
import { MpptchgService } from './mpptchg.service';
import { RadioService } from './radio.service';
import { DatabaseService } from './database.service';
import { Logs } from '../models/logs';
import { ProcessService } from './process.service';
import { CommunicationMpptchdService } from './communication-mpptchd.service';
import { exec } from 'child_process';

export class DashboardService {

    private readonly app: express.Application = express();

    constructor(config: ConfigInterface) {
        this.app.use(express.json());

        this.app.use((req, res, next) => {
            if (!req.path.startsWith('/assets')) {
                LogService.log('dashboard', 'Request start', req.method, req.path);
            }

            res.header('Access-Control-Allow-Origin', '*');
            res.header('Access-Control-Allow-Headers', 'Origin, X-Requested-With, Content-Type, Accept');

            res.on('finish', () => {
                if (!req.path.includes('assets')) {
                    LogService.log('dashboard', 'Request end', req.method, req.path, res.statusCode);
                }
            });

            if (req.path.startsWith('/api')) {
                if (config.dashboard.apikey && req.query.apikey !== config.dashboard.apikey) {
                    res.sendStatus(403);
                    return;
                }
            }

            next();
        });

        this.app.options('*', (req, res) => {
            res.header('Access-Control-Allow-Methods', 'GET, PATCH, PUT, POST, DELETE, OPTIONS');
            res.send();
        });

        this.app.use('/assets', express.static(__dirname + '/../../assets/dashboard/assets'));
        this.app.use('/data.db', express.static(config.databasePath));

        this.app.get('/', (req, res) => {
            forkJoin([SensorsService.getLast(), WebcamService.getLastPhotos(config.webcam)])
                .subscribe(datas => {
                    res.render(__dirname + '/../../assets/dashboard/index.ejs', {
                        sensors: datas[0],
                        lastPhoto: datas[1].length > 0 ? datas[1][0] : null
                    });
                });
        });

        this.app.get('/photos', (req, res) => {
            WebcamService.getLastPhotos(config.webcam).subscribe(photos => {
                res.render(__dirname + '/../../assets/dashboard/photos.ejs', {
                    photos: photos
                });
            });
        });

        this.app.get('/stats', (req, res) => {
            res.render(__dirname + '/../../assets/dashboard/stats.ejs');
        });

        this.app.get('/admin', (req, res) => {
            res.render(__dirname + '/../../assets/dashboard/admin.ejs', {
                apikey: config.debug ? config.dashboard.apikey : ''
            });
        });

        this.app.get('/last.json', (req, res) => {
            forkJoin([SensorsService.getLast(), WebcamService.getLastPhotos(config.webcam)])
                .subscribe(datas => {
                    res.json({
                        sensors: datas[0],
                        lastPhoto: datas[1].length > 0 ? datas[1][0] : null
                    });
                });
        });

        this.app.get('/api/logs', (req, res) => {
            DatabaseService.selectAll(Logs.name, 500).subscribe((logs: Logs[]) => {
                logs.forEach(log => {
                    (log as any).createdAt = new Date(log.createdAt);
                    log.data = JSON.parse(log.data);
                })
                res.send(logs);
            });
        });

        this.app.post('/api/do-not-shutdown/:value', (req, res) => {
            ProcessService.doNotShutdown = req.params['value'] === '1';
            res.send(true);
        });

        this.app.post('/api/program/stop', (req, res) => {
            ProcessService.doNotShutdown = true;
            exec('systemctl stop logic');
            process.exit(0);
            res.send(true);
        });

        this.app.post('/api/program/restart', (req, res) => {
            ProcessService.doNotShutdown = true;
            exec('systemctl restart logic');
            process.exit(0);
            res.send(true);
        });

        this.app.post('/api/gpio/:pin/:value', (req: Request, res) => {
            const pin = GpioEnum[req.params['pin']];
            const value = req.params['value'] === '1';

            if (pin === GpioEnum.RelayRadio) {
                RadioService.keepOn = value;
                LogService.log('radio', 'State keepOn set by dashboard', RadioService.keepOn);
            }

            GpioService.set(pin, value).pipe(
                catchError(e => {
                    res.json(e);
                    return of(null);
                })
            ).subscribe(_ => res.send(true));
        });

        this.app.get('/api/config', (req, res) => {
            res.send(config);
        });

        if (config.sensors && config.sensors.enable) {
            this.app.get('/api/sensors', (req, res) => {
                SensorsService.getAllCurrent().subscribe(datas => {
                    (datas as any).createdAt = new Date(datas.createdAt);
                    datas.rawMpptchg = JSON.parse(datas.rawMpptchg);
                    res.send(datas);
                });
            });

            this.app.use('/sensors.csv', express.static(config.sensors.csvPath));
            this.app.use('/sensors.json', (req, res) => {
                SensorsService.getAllSaved(500).subscribe(datas => {
                    res.send(datas.map(data => {
                        return {
                            createdAt: data.createdAt,
                            voltageBattery: data.voltageBattery,
                            voltageSolar: data.voltageSolar,
                            currentCharge: data.currentCharge,
                            currentBattery: data.currentBattery,
                            currentSolar: data.currentSolar,
                            temperature: (data.temperatureBattery + data.temperatureRtc) / 2
                        }
                    }));
                });
            });
        }

        if (config.mpptChd && config.mpptChd.enable) {
            this.app.post('/api/shutdown/{timestamp}', (req, res) => {
                const restartDate = new Date(+req.params['timestamp'] * 1000);
                MpptchgService.shutdownAndWakeUpAtDate(restartDate).pipe(
                    catchError(e => {
                        res.json(e);
                        return of(null);
                    })
                ).subscribe(wakeupDate => res.send(wakeupDate));
            });

            this.app.post('/api/watchdog/stop', (req, res) => {
                MpptchgService.stopWatchdog().pipe(
                    catchError(e => {
                        res.json(e);
                        return of(null);
                    })
                ).subscribe(_ => res.send(true));
            });

            if (config.mpptChd.watchdog) {
                this.app.post('/api/watchdog/start', (req, res) => {
                    MpptchgService.startWatchdog().pipe(
                        catchError(e => {
                            res.json(e);
                            return of(null);
                        })
                    ).subscribe(_ => res.send(true));
                });
            }
        }

        if (config.repeater && config.repeater.enable) {
            this.app.post('/api/repeater', (req, res) => {
                RadioService.listenAndRepeat(config.repeater.seconds).pipe(
                    catchError(e => {
                        res.json(e);
                        return of(null);
                    })
                ).subscribe(_ => res.send(true));
            });
        }

        if (config.sstv && config.sstv.enable) {
            this.app.post('/api/sstv', (req, res) => {
                SstvService.sendImage(config.sstv).pipe(
                    catchError(e => {
                        res.json(e);
                        return of(null);
                    })
                ).subscribe(_ => res.send(true));
            });
        }

        if (config.webcam) {
            if (config.webcam.enable) {
                this.app.post('/api/webcam', (req, res) => {
                    WebcamService.captureAndSend(config.webcam, config?.sftp).pipe(
                        catchError(e => {
                            res.json(e);
                            return of(null);
                        })
                    ).subscribe(filePath => res.sendFile(filePath));
                });
            }

            this.app.use('/timelapse', express.static(config.webcam.photosPath));
        }

        if (config.voice && config.voice.enable) {
            this.app.post('/api/voice', (req, res) => {
                CommunicationMpptchdService.instance.getStatus().pipe(
                    map(data => {
                        return config.voice.sentence
                            .replace('batteryVoltage', data.values.batteryVoltage / 1000 + '')
                            .replace('chargeCurrent', data.values.chargeCurrent + '')
                            ;
                    }),
                    switchMap(sentence => VoiceService.sendVoice(sentence, true, config.voice)),
                    catchError(e => {
                        res.json(e);
                        return of(null);
                    })
                ).subscribe(_ => res.send(true));
            });
        }

        if (config.aprs && config.aprs.enable) {
            this.app.post('/api/aprs/beacon', (req, res) => {
                AprsService.sendAprsBeacon(config.aprs).pipe(
                    catchError(e => {
                        res.json(e);
                        return of(null);
                    })
                ).subscribe(_ => res.send(true));
            });

            this.app.post('/api/aprs/telemetry', (req, res) => {
                AprsService.sendAprsTelemetry(config.aprs, config.sensors).pipe(
                    catchError(e => {
                        res.json(e);
                        return of(null);
                    })
                ).subscribe(_ => res.send(true));
            });
        }

        const port = config.dashboard.port ?? 3000;
        this.app.listen(port, () => {
            LogService.log('dashboard', 'Started', port);
        });
    }
}
