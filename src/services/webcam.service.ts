import NodeWebcam = require('node-webcam');
import fs = require('fs');
import { Observable, Observer, of } from 'rxjs';
import { LogService } from './log.service';
import { WebcamConfigInterface } from '../config/webcam-config.interface';
import { assetsFolder } from '../index';

export class WebcamService {

    public static USE_FAKE = false;
    public static lastPhotoPath: string;

    private static readonly opts = {
        width: 640,
        height: 480,
        quality: 100,
        skip: 100,
        // delay: 10,
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
                const errors = [];

                WebcamService.webcam.list(list => {
                    let ended = false;
                    list.reverse().forEach(cam => {
                        if (!ended) {
                            WebcamService.opts.device = cam;
                            WebcamService.webcam = NodeWebcam.create(WebcamService.opts);

                            const date = new Date();
                            // @ts-ignore
                            let path = configWebcam.photosPath + '/' + date.format('{YYYY}_{MM}_{DD}');

                            try {
                                if (!fs.existsSync(path)) {
                                    fs.mkdirSync(path);
                                }
                            } catch (e) {
                                LogService.log('webcam', 'Creation path error', path);
                                path = configWebcam.photosPath;
                            }

                            // @ts-ignore
                            WebcamService.webcam.capture(path + '/' + date.format('{YYYY}_{MM}_{DD}-{hh}_{mm}_{ss}'), (err, data) => {
                                WebcamService.alreadyInUse = false;

                                if (err) {
                                    errors.push(err);
                                } else if (!fs.existsSync(data)) {
                                    errors.push(new Error('File not found (webcam taken : ' + cam + ')'));
                                } else {
                                    LogService.log('webcam', 'Capture OK', data);

                                    WebcamService.lastPhotoPath = data;
                                    observer.next(data);
                                    observer.complete();
                                    ended = true;
                                }
                            });
                        }
                    });
                });

                if (errors.length) {
                    LogService.log('webcam', 'Capture error', errors);
                    observer.error(errors);
                }
            } else {
                WebcamService.alreadyInUse = false;
                LogService.log('webcam', 'Capture fake OK');

                WebcamService.lastPhotoPath = assetsFolder + '/test.jpg';
                observer.next(WebcamService.lastPhotoPath);
                observer.complete();
            }
        });
    }

    public static getLastPhotos(config: WebcamConfigInterface): Observable<string[]> {
        return new Observable<string[]>((observer: Observer<string[]>) => {
            fs.readdir(config.photosPath, (err, datesDir) => {
                if (err) {
                    observer.error(err);
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

                    const filesSortedDesc = allFiles.sort((a, b) => a < b ? 1 : -1);

                    if (!WebcamService.lastPhotoPath && filesSortedDesc.length > 0) {
                        WebcamService.lastPhotoPath = filesSortedDesc[0];
                    }

                    observer.next(filesSortedDesc);
                } else {
                    observer.next(null);
                }
                observer.complete();
            });
        });
    }
}
