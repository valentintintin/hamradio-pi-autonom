import { FeatureInterface } from './config.interface';

export class ApiConfigInterface implements FeatureInterface {
    enable: boolean;
    port?: number;
}
