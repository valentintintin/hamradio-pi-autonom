using System.Text.Json.Serialization;
using Monitor.Models.SerialMessages;

namespace Monitor.Models;

public class LoraState
{
    [JsonIgnore]
    public LimitedList<LoraData> LastTx { get; } = new(20);
    
    [JsonIgnore]
    public LimitedList<LoraData> LastRx { get; } = new(20);
    
    public List<LoraData> All => 
        LastTx
        .Concat(LastRx)
        .OrderBy(a => a.ReceivedAt)
        .ToList();
}