import { FeatureInterface } from './config.interface';

export interface SftpConfigInterface extends FeatureInterface {
    host: string;
    username: string;
    remotePath: string;
    privateKeyPath: string;
    privateKeyPassphrase: string;
    interval?: number;
}
