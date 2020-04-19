import { Entity } from './entity';

export class Logs extends Entity {
    public createdAt: number = new Date().getTime();
    public service: string;
    public log: string;
    public data: string;
}

