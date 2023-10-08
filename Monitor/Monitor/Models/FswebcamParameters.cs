namespace Monitor.Models;

using System.Text;

public class FswebcamParameters
{
    public required string Device { get; set; }
    public required string Resolution { get; set; }
    public string? OutputLogFile { get; set; }
    public bool? NoBanner { get; set; }
    public bool? Quiet { get; set; }
    public string? Input { get; set; }
    public int? Delay { get; set; }
    public int? Fps { get; set; }
    public int? Frames { get; set; }
    public int? Skip { get; set; }
    public bool? Revert { get; set; }
    public string? Flip { get; set; }
    public string? Crop { get; set; }
    public string? Scale { get; set; }
    public int? Rotate { get; set; }
    public bool? Deinterlace { get; set; }
    public bool? Invert { get; set; }
    public bool? Greyscale { get; set; }
    public string? SwapChannels { get; set; }
    public bool? TopBanner { get; set; }
    public bool? BottomBanner { get; set; }
    public string? BannerColour { get; set; }
    public string? LineColour { get; set; }
    public string? TextColour { get; set; }
    public string? Font { get; set; }
    public bool? Shadow { get; set; }
    public string? Title { get; set; }
    public string? Subtitle { get; set; }
    public string? Timestamp { get; set; }
    public bool? Gmt { get; set; }
    public string? Info { get; set; }
    public string? Underlay { get; set; }
    public string? Overlay { get; set; }
    public int? JpegFactor { get; set; }
    public int? PngFactor { get; set; }
    public required string SaveFile { get; set; }
    public string? ExecCommand { get; set; }

    public override string ToString()
    {
        StringBuilder arguments = new();

        // Append properties with non-null values to the arguments string
        if (!string.IsNullOrWhiteSpace(Device)) arguments.Append($"-d {Device} ");
        if (!string.IsNullOrWhiteSpace(Resolution)) arguments.Append($"-r {Resolution} ");
        if (!string.IsNullOrWhiteSpace(OutputLogFile)) arguments.Append($"-o {OutputLogFile} ");
        if (NoBanner.HasValue && NoBanner.Value) arguments.Append("--no-banner ");
        if (Quiet.HasValue && Quiet.Value) arguments.Append("-q ");
        if (!string.IsNullOrWhiteSpace(Input)) arguments.Append($"-i {Input} ");
        if (Delay.HasValue) arguments.Append($"-D {Delay} ");
        if (Fps.HasValue) arguments.Append($"--fps {Fps} ");
        if (Frames.HasValue) arguments.Append($"-F {Frames} ");
        if (Skip.HasValue) arguments.Append($"-S {Skip} ");
        if (Revert.HasValue && Revert.Value) arguments.Append("--revert ");
        if (!string.IsNullOrWhiteSpace(Flip)) arguments.Append($"--flip {Flip} ");
        if (!string.IsNullOrWhiteSpace(Crop)) arguments.Append($"--crop {Crop} ");
        if (!string.IsNullOrWhiteSpace(Scale)) arguments.Append($"--scale {Scale} ");
        if (Rotate.HasValue) arguments.Append($"--rotate {Rotate} ");
        if (Deinterlace.HasValue && Deinterlace.Value) arguments.Append("--deinterlace ");
        if (Invert.HasValue && Invert.Value) arguments.Append("--invert ");
        if (Greyscale.HasValue && Greyscale.Value) arguments.Append("--greyscale ");
        if (!string.IsNullOrWhiteSpace(SwapChannels)) arguments.Append($"--swapchannels {SwapChannels} ");
        if (TopBanner.HasValue && TopBanner.Value) arguments.Append("--top-banner ");
        if (BottomBanner.HasValue && BottomBanner.Value) arguments.Append("--bottom-banner ");
        if (!string.IsNullOrWhiteSpace(BannerColour)) arguments.Append($"--banner-colour {BannerColour} ");
        if (!string.IsNullOrWhiteSpace(LineColour)) arguments.Append($"--line-colour {LineColour} ");
        if (!string.IsNullOrWhiteSpace(TextColour)) arguments.Append($"--text-colour {TextColour} ");
        if (!string.IsNullOrWhiteSpace(Font)) arguments.Append($"--font {Font} ");
        if (Shadow.HasValue && Shadow.Value) arguments.Append("--shadow ");
        if (!string.IsNullOrWhiteSpace(Title)) arguments.Append($"--title {Title} ");
        if (!string.IsNullOrWhiteSpace(Subtitle)) arguments.Append($"--subtitle {Subtitle} ");
        if (!string.IsNullOrWhiteSpace(Timestamp)) arguments.Append($"--timestamp {Timestamp} ");
        if (Gmt.HasValue && Gmt.Value) arguments.Append("--gmt ");
        if (!string.IsNullOrWhiteSpace(Info)) arguments.Append($"--info {Info} ");
        if (!string.IsNullOrWhiteSpace(Underlay)) arguments.Append($"--underlay {Underlay} ");
        if (!string.IsNullOrWhiteSpace(Overlay)) arguments.Append($"--overlay {Overlay} ");
        if (JpegFactor.HasValue) arguments.Append($"--jpeg {JpegFactor} ");
        if (PngFactor.HasValue) arguments.Append($"--png {PngFactor} ");
        if (!string.IsNullOrWhiteSpace(SaveFile)) arguments.Append($"--save {SaveFile} ");
        if (!string.IsNullOrWhiteSpace(ExecCommand)) arguments.Append($"--exec {ExecCommand} ");

        return arguments.ToString().Trim();
    }
}