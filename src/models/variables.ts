import { Entity } from './entity';

export class Variables extends Entity {
    public updatedAt: number = new Date().getTime();
    public name: string;
    public data: string;
}

export enum EnumVariable {
    SEQ_TELEMETRY = 'seq_telemetry'
}
