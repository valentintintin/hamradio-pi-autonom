namespace Monitor.Models;

public class StatsView
{
    public class UnitData
    {
        public DateTime DateTime { get; set; }
        
        public double Value { get; set; }
    }
    
    public class Data
    {
        public double Average { get; set; }
        public double Min { get; set; }
        public double Max { get; set; }

        public void FromData(IQueryable<UnitData> data)
        {
            if (data.Any())
            {
                Average = Math.Round(data.Average(d => d.Value), 2);
                Min = Math.Round(data.Min(d => d.Value), 2);
                Max = Math.Round(data.Max(d => d.Value), 2);
            }
        }
    }
    
    public Data Total { get; set; } = new();
    public Data TotalToday { get; set; } = new();
    public Data TotalYesterday { get; set; } = new();

    public void FromData(IQueryable<UnitData> data)
    {
        Total.FromData(data);
        
        var today = DateTime.Today;
        var yesterday = DateTime.Today.AddDays(-1);
        
        TotalToday.FromData(data.Where(d => d.DateTime.Date == today));
        TotalYesterday.FromData(data.Where(d => d.DateTime.Date == yesterday));
    }
}