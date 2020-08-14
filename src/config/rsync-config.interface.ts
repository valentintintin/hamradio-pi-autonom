import { FeatureInterface } from './config.interface';

export interface RsyncConfigInterface extends FeatureInterface {
    host: string;
    username: string;
    remotePath: string;
    privateKeyPath: string;
    interval?: number;
}
