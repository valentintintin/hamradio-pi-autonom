namespace Monitor.Models;

public class LoraState
{
    public LimitedList<string> LastTx { get; } = new(20);

    public LimitedList<string> LastRx { get; } = new(20);
}