export abstract class Entity {

    public id: number;
    public createdAt: number = new Date().getTime();
}
