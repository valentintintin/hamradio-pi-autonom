namespace Monitor.Models;

public class LoraState
{
    public LimitedList<(string payload, DateTime date)> LastTx { get; } = new(20);

    public LimitedList<(string payload, DateTime date)> LastRx { get; } = new(20);

    public List<(string payload, DateTime date, bool isTx)> All => 
        LastTx.Select(a => (a.payload, a.date, true))
        .Concat(LastRx.Select(a => (a.payload, a.date, false)))
        .OrderByDescending(a => a.date)
        .ToList();
}