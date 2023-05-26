import { FeatureIntervalInterface } from './config.interface';

export interface WebcamConfigInterface extends FeatureIntervalInterface {
    photosPath: string;
    fake?: boolean;
}
