import NodeWebcam = require('node-webcam');
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

    private static readonly webcam = NodeWebcam.create({
        width: 640,
        height: 480,
        quality: 100,
        skip: 100,
        saveShots: true,
        output: 'jpeg',
        callbackReturn: 'location',
        verbose: false
    });

    public static alreadyInUse: boolean;

    public static captureAndSend(configWebcam: WebcamConfigInterface, configSftp: SftpConfigInterface): Observable<string> {
        LogService.log('webcam', 'Start capturing');

        if (WebcamService.alreadyInUse) {
            LogService.log('webcam', 'Already in use');
            return of(null);
        }

        WebcamService.alreadyInUse = true;

        return new Observable<string>((observer: Observer<string>) => {
            if (!WebcamService.USE_FAKE) {
                // @ts-ignore
                this.webcam.capture(configWebcam.photosPath + '/' + new Date().format('{YYYY}_{MM}_{DD}-{hh}_{mm}_{ss}'), (err, data) => {
                    WebcamService.alreadyInUse = false;

                    if (err) {
                        LogService.log('webcam', 'Capture error', err);
                        observer.error(err);
                    } else {
                        LogService.log('webcam', 'Capture OK');

                        WebcamService.lastPhotoPath = data;
                        observer.next(data);
                        observer.complete();
                    }
                });
            } else {
                WebcamService.alreadyInUse = false;
                LogService.log('webcam', 'Capture OK');

                WebcamService.lastPhotoPath = assetsFolder + '/test.jpg';
                observer.next(WebcamService.lastPhotoPath);
                observer.complete();
            }
        }).pipe(
            switchMap(photoPath => SftpService.send(configSftp, photoPath).pipe(
                catchError(_ => of(photoPath)),
                switchMap(_ => of(photoPath))
            ))
        );
    }
}
