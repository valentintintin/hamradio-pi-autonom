import NodeWebcam = require('node-webcam');
import fs = require('fs');
import { Observable, Observer, of } from 'rxjs';
import { LogService } from './log.service';
import { WebcamConfigInterface } from '../config/webcam-config.interface';
import { assetsFolder } from '../index';
import { catchError, switchMap } from 'rxjs/operators';
import { SftpService } from './sftp.service';
import { SftpConfigInterface } from '../config/sftp-config.interface';

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

                            // @ts-ignore
                            WebcamService.webcam.capture(configWebcam.photosPath + '/' + new Date().format('{YYYY}_{MM}_{DD}-{hh}_{mm}_{ss}'), (err, data) => {
                                WebcamService.alreadyInUse = false;

                                if (err) {
                                    errors.push(err);
                                } else if (!fs.existsSync(data)) {
                                    errors.push(new Error('File not found (webcam taken : ' + cam + ')'));
                                } else {
                                    LogService.log('webcam', 'Capture OK');

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

    public static captureAndSend(configWebcam: WebcamConfigInterface, configSftp: SftpConfigInterface): Observable<string> {
        return WebcamService.capture(configWebcam).pipe(
            switchMap(photoPath => SftpService.send(configSftp, photoPath).pipe(
                catchError(_ => of(photoPath)),
                switchMap(_ => of(photoPath))
            ))
        );
    }

    public static getLastPhotos(config: WebcamConfigInterface): Observable<string[]> {
        return new Observable<string[]>((observer: Observer<string[]>) => {
            fs.readdir(config.photosPath, (err, files) => {
                if (err) {
                    observer.error(err);
                }

                const filesSortedDesc = files.sort((a, b) => a < b ? 1 : -1);

                if (!WebcamService.lastPhotoPath && filesSortedDesc.length > 0) {
                    WebcamService.lastPhotoPath = filesSortedDesc[0];
                }

                observer.next(filesSortedDesc);
                observer.complete();
            });
        });
    }
}
