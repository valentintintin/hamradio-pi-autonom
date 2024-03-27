namespace Monitor.Models;

public class ImageMagickParameters
{
    private readonly List<string> _commands = [];
    
    private int? _translateX = 0;
    private int? _translateY = 0;
    
    public ImageMagickParameters CreateCanvas(int width, int height, string colorName)
    {
        _commands.Add($"-size {width}x{height} xc:{colorName}");
        return this;
    }
    
    public ImageMagickParameters Output(string outputFile = "jpeg:-")
    {
        _commands.Add($"{outputFile}");
        return this;
    }

    public ImageMagickParameters Rotate(int degrees)
    {
        _commands.Add($"-rotate {degrees}");
        return this;
    }

    public ImageMagickParameters SetQuality(int quality)
    {
        _commands.Add($"-quality {quality}");
        return this;
    }

    public ImageMagickParameters SetFormat(string format)
    {
        _commands.Add($"-format {format}");
        return this;
    }

    public ImageMagickParameters Translate(int x, int y)
    {
        _translateX += x;
        _translateY += y;
        return this;
    }

    public ImageMagickParameters AddText(int fontSize, string fontPath, int x, int y, string text, string textColor)
    {
        _commands.Add($"-font {fontPath} -pointsize {fontSize} -stroke {textColor} -strokewidth 1 -fill {textColor} -annotate +{_translateX + x}+{_translateY + y} \"{text}\"");
        return this;
    }

    public ImageMagickParameters OverlayImage(string overlayImagePath, int x, int y)
    {
        _commands.Add($"-draw \"image over {_translateX + x},{_translateY + y} 0,0 '{overlayImagePath}'\"");
        return this;
    }

    public ImageMagickParameters DrawRectangle(int x, int y, int width, int height, string color)
    {
        _commands.Add($"-fill {color} -stroke {color} -draw \"rectangle {_translateX + x},{_translateY + y - height} {_translateX + x + width},{_translateY + y}\"");
        return this;
    }

    public ImageMagickParameters AddCustomArgument(string argument)
    {
        _commands.Add(argument);
        return this;
    }

    public override string ToString()
    {
        return string.Join(" ", _commands);
    }
}