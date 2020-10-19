import NodeWebcam = require('node-webcam');
import fs = require('fs');
import { Observable, Observer, of } from 'rxjs';
import { LogService } from './log.service';
import { WebcamConfigInterface } from '../config/webcam-config.interface';
import { switchMap } from 'rxjs/operators';
import { DatabaseService } from './database.service';
import { EnumVariable } from '../models/variables';

export class WebcamService {

    public static USE_FAKE = false;

    private static readonly opts = {
        width: 640,
        height: 480,
        quality: 100,
        skip: 0,
        saveShots: true,
        output: 'jpeg',
        callbackReturn: 'location',
        verbose: false,
        device: false
    };
    private static webcam = NodeWebcam.create(WebcamService.opts);

    public static alreadyInUse: boolean;

    public static capture(configWebcam: WebcamConfigInterface): Observable<string> {
        LogService.log('webcam', 'Start capturing');

        if (WebcamService.alreadyInUse) {
            LogService.log('webcam', 'Already in use');
            return of(null);
        }

        WebcamService.alreadyInUse = true;

        return new Observable<string>((observer: Observer<string>) => {
            if (!WebcamService.USE_FAKE) {
                const now = new Date();
                // @ts-ignore
                const dirName = now.format('{YYYY}_{MM}_{DD}');
                // @ts-ignore
                const fileName = now.format('{YYYY}_{MM}_{DD}-{hh}_{mm}_{ss}');
                let path = '';

                try {
                    path = configWebcam.photosPath + '/' + dirName;
                    if (!fs.existsSync(path)) {
                        fs.mkdirSync(path);
                    }

                    path += '/' + fileName;
                } catch (e) {
                    LogService.log('webcam', 'Creation path error', path);
                    path = configWebcam.photosPath + '/' + fileName;
                }

                let nb = 0;
                WebcamService.webcam.list(list => {
                    list.forEach((cam: string, index: number) => {
                        if (cam.endsWith('2')) {
                            WebcamService.opts.skip = 2;
                        } else if (cam.endsWith('4')) {
                            WebcamService.opts.skip = 150;
                        } else {
                            return;
                        }

                        (WebcamService.opts as any).device = cam;
                        WebcamService.webcam = NodeWebcam.create(WebcamService.opts);

                        let pathWebcam = path;
                        pathWebcam += '_' + index;

                        WebcamService.webcam.capture(pathWebcam, (err, data: string) => {
                            nb++;
                            if (err) {
                                LogService.log('webcam', 'Capture KO', data, err);
                                observer.error(err);
                            } else if (nb === 2) {
                                WebcamService.alreadyInUse = false;

                                LogService.log('webcam', 'Capture OK', data);
                                LogService.consoleLog('webcam', 'Last finished');

                                const log: LastPhotoInterface = {
                                    date: new Date().getTime(),
                                    path: '/timelapse/' + dirName + '/' + fileName
                                }
                                DatabaseService.updateVariable(EnumVariable.LAST_PHOTO, JSON.stringify(log)).subscribe();

                                observer.next(data);
                                observer.complete();
                            } else {
                                LogService.log('webcam', 'Capture OK', data);
                                observer.next(data);
                            }
                        });
                    });
                });
            } else {
                WebcamService.alreadyInUse = false;
                LogService.log('webcam', 'Capture fake OK');

                const log: LastPhotoInterface = {
                    date: new Date().getTime(),
                    path: '/assets/test.jpg'
                };

                DatabaseService.updateVariable(EnumVariable.LAST_PHOTO, JSON.stringify(log)).subscribe();
                observer.next(log.path);
                observer.complete();
            }
        });
    }

    public static getLastPhoto(): Observable<LastPhotoInterface> {
        return DatabaseService.readVariable(EnumVariable.LAST_PHOTO).pipe(
            switchMap((d: string) => {
                const lastPhoto: LastPhotoInterface = JSON.parse(d);
                return of(lastPhoto);
            })
        );
    }

    public static getLastPhotos(config: WebcamConfigInterface): Observable<string[]> {
        return new Observable<string[]>((observer: Observer<string[]>) => {
            fs.readdir(config.photosPath, (err, datesDir) => {
                if (err) {
                    observer.error(err);
                    return;
                }

                if (datesDir) {
                    let allFiles = [];
                    datesDir.forEach(dateDir => {
                        try {
                            const dateFiles = fs.readdirSync(config.photosPath + '/' + dateDir);
                            allFiles = allFiles.concat(dateFiles.map(f => dateDir + '/' + f));
                        } catch (e) {
                        }
                    });

                    observer.next(allFiles.sort((a, b) => a < b ? 1 : -1));
                } else {
                    observer.next(null);
                }
                observer.complete();
            });
        });
    }
}

export interface LastPhotoInterface {
    path: string;
    date: number;
}
