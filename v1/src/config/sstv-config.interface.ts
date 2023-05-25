import { FeatureInterface } from './config.interface';

export interface SstvConfigInterface extends FeatureInterface {
    comment: string;
    mode: 'm1' | 'm2' | 's1' | 's2' | 'sdx' | 'r36';
    dtmfCode: string;
    pisstvPath: string;
}
