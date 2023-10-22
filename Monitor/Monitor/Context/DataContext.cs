using Microsoft.EntityFrameworkCore;
using Microsoft.EntityFrameworkCore.ChangeTracking;
using Monitor.Context.Entities;

namespace Monitor.Context;

public class DataContext : DbContext
{
    public required DbSet<Weather> Weathers { get; set; }
    public required DbSet<Mppt> Mppts { get; set; }
    
    public required DbSet<Entities.System> Systems { get; set; }
    public required DbSet<LoRa> LoRas { get; set; }
    public required DbSet<Config> Configs { get; set; }
    
    public DataContext(DbContextOptions<DataContext> options) : base(options) {}

    public override int SaveChanges(bool acceptAllChangesOnSuccess)
    {
        ComputeEntitiesBeforeSaveChanges();
        
        return base.SaveChanges(acceptAllChangesOnSuccess);
    }
    
    public override Task<int> SaveChangesAsync(
        bool acceptAllChangesOnSuccess,
        CancellationToken cancellationToken = default)
    {
        ComputeEntitiesBeforeSaveChanges();
        
        return base.SaveChangesAsync(acceptAllChangesOnSuccess, cancellationToken);
    }

    private void ComputeEntitiesBeforeSaveChanges()
    {
        foreach (EntityEntry entityEntry in ChangeTracker.Entries())
        {
            if (entityEntry.Entity is IEntity entity)
            {
                switch (entityEntry.State)
                {
                    case EntityState.Added:
                        entity.CreatedAt = DateTime.UtcNow;
                        break;
                }
            }
        }
    }
}