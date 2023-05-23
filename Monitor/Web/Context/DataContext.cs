using Microsoft.EntityFrameworkCore;
using Web.Context.Entities;

namespace Web.Context;

public class DataContext : DbContext
{
    public required DbSet<Weather> Weathers { get; set; }
    
    public required DbSet<Mppt> Mppts { get; set; }
    
    public DataContext(DbContextOptions<DataContext> options) : base(options) {}
}