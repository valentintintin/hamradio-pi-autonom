import NodeWebcam = require('node-webcam');
import { Observable, Observer } from 'rxjs';
import { LogService } from './log.service';
import { WebcamConfigInterface } from '../config/webcam-config.interface';
import { assetsFolder } from '../index';

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

    public static capture(config: WebcamConfigInterface): Observable<string> {
        return new Observable<string>((observer: Observer<string>) => {
            LogService.log('webcam', 'Start capturing');

            if (!WebcamService.USE_FAKE) {
                // @ts-ignore
                this.webcam.capture(config.photosPath + '/' + new Date().format('{YYYY}_{MM}_{DD}-{hh}_{mm}_{ss}'), (err, data) => {
                    if (err) {
                        LogService.log('webcam', 'Capture error', err);
                        observer.error(err);
                    } else {
                        WebcamService.lastPhotoPath = data;
                        observer.next(data);
                        observer.complete();
                    }
                });
            } else {
                WebcamService.lastPhotoPath = assetsFolder + '/test.jpg';
                observer.next(WebcamService.lastPhotoPath);
                observer.complete();
            }
        });
    }
}
