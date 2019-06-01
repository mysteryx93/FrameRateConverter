# FrameRateConverter
Increases the frame rate with interpolation and fine artifact removal.

by Etienne Charland


## FrameRateConverter

Increases the frame rate with interpolation and fine artifact removal.

FrameRateConverter(C, NewNum, NewDen, Preset, BlkSize, BlkSizeV, FrameDouble, Output, Debug, Prefilter, MaskThr, MaskOcc, SkipThr, BlendOver, SkipOver, Stp, Dct, DctRe)

YV12/YV24/Y8/YUY2  
Requires: FrameRateConverter.dll, MaskTools2, MvTools2 (pinterf), GRunT (for debug only)

@ NewNum      - The new framerate numerator (if FrameDouble = false, default = 60)

@ NewDen      - The new framerate denominator (if FrameDouble = false, default = 1)

@ Preset      - The speed/quality preset [slowest|slower|slow|normal|fast|faster]. (default=normal)

@ BlkSize     - The block size. Latest MvTools2.dll version from Pinterf supports 6, 8, 12, 16, 24, 32, 48 and 64.  
                Defaults for 4/3 video of height:  
                0-359:  8  
                360-749: 12  
                750-1199: 16  
                1200-1699: 24  
                1600-2160: 32  

@ BlkSizeV    - The vertical block size. (default = BlkSize)

@ FrameDouble - Whether to double the frame rate and preserve original frames (default = true)

@ Output      - Output mode [auto|flow|over|none|raw|mask|skip|diff|stripe] (default = auto)
                auto=normal artifact masking; flow=interpolation only; over=mask as cyan overlay, stripes mask as yellow; none=ConvertFPS only; raw=raw mask; 
                mask=mask only; skip=mask used to Skip; diff=mask where alternate interpolation is better; stripe=mask used to cover stripes

@ Debug       - Whether to display AverageLuma values of Skip, Mask and Raw. (Default = false)

@ Prefilter   - Specifies a prefilter such as RgTools' RemoveGrain(21). Recommended only when not using a denoiser (Default=none)

@ MaskThr     - The threshold where a block is considered bad, between 0 and 255. Smaller = stronger.
                0 to disable artifact masking. (Default = 120)

@ MaskOcc     - Occlusion mask threshold, between 0 and 255. 0 to disable occlusion masking. (Default = 105)

@ SkipThr     - The threshold where a block is counted for the skip mask, between 0 and 255. Smaller = stronger.
                Must be smaller (stronger) than MaskThr. (Default = 55)

@ BlendOver   - Try fallback block size when artifacts cover more than specified threshold, or 0 to disable.
                If it fails again, it will revert to frame blending. (default = 70)

@ SkipOver    - Skip interpolation of frames when artifacts cover more than specified threshold, 
                or 0 to disable. (Default = 210)
                
@ Stp         - Whether to detect and blend stripes (default=true)

@ Dct         - Overrides DCT parameter for MAnalyse (default: Normal=0, Slow=4, Slowest=1)

@ DctRe       - Overrides DCT parameter for MRecalculate (default: Fast=0, Normal=4, Slowest=1)
                
@ BlendRatio  - Changes the blend ratio used to fill artifact zones. 0 = frame copy and 100 = full blend.
                Other values provide a result in-between to eliminate ghost effects. Default = 40.


Presets  
Faster:  Basic interpolation
Fast:    MRecalculate
Normal:  MRecalculate with DCT=4
Slow:    MAnalyze + MRecalculate with DCT=4
Slower:  Calculate diff between DCT=4 and DCT=0 to take the best from both
Slowest: Calculate diff between DCT=1 and DCT=0 to take the best from both
Anime:   Slow with BlendOver=40, SkipOver=140




## InterpolateDoubles

Replace double frames with interpolated frames using FrameRateConverter

InterpolateDoubles(C, Thr, Show, Preset, BlkSize, BlkSizeV, MaskThr, MaskOcc, SkipThr, BlendOver, SkipOver, Stripes, Dct, DctRe)

@ Thr         - Frames will be replaced when Luma difference with previous frame is greater than threshold (default=.1)

@ Show        - If true, "FRAME FIXED" will be written on replaced frames (default=false)

@ All other parameters are the same as FrameRateConverter




## StripeMask

Builds a mask detecting horizontal and vertical straight lines and patterns, as MvTools tends to fail in such areas.

StripeMask(C, BlkSize, BlkSizeV, Overlap, OverlapV, Thr, Comp, CompV, Str, StrF, Lines

@ BlkSize, BlkSizeV     - The horizontal and vertical block size. (default: BlkSize=16, BlkSizeV=BlkSize)

@ Overlap, OverlapV     - How many pixels to overlap between blocks, generally between 1/4 and 1/2 of block size. (default = BlkSize/4)

@ Thr                   - Dynamic content gives blended (grey) line averages while lines and stripes have contrast between average values. This specifies the contrast threshold where a line average is taken for calculations. A lower value gives a stronger and more sensitive mask. (default = 26)

@ Comp, CompV           - How many lines averages to compare with each other. (default = 2 with BlkSize<16 and 3 with BlkSize>=16)

@ Str                   - The value to set on the mask when a pattern is detected, between 0 and 255. (default = 255)

@ StrF                  - If > 0, calculates the next frame and set its patterns to this value, between 0 and 255. (default = 0)

@ Lines                 - If true, display the raw contrast lines being used for calculations. If false, the pattern areas between those lines will be marked. (default = false)




## ConvertFpsLimit

Same as ConvertFps but with an extra parameter:

@ Ratio      - Changes the blend ratio. 0 = frame copy and 100 = full blend.
               Other values provide a result in-between to eliminate ghost effects. Default = 100.




## ConditionalFilterMT

[Avisynth+ MT](https://forum.doom9.org/showthread.php?t=168856) provides great capabilities to process videos. However, conditional functions are
not compatible with MT (multi-threading) due to design limitations. To work around this problem,
this class provides a subset of conditional features that will work with MT mode.

Currently supported:
- [ConditionalFilter](http://avisynth.nl/index.php/ConditionalFilter)

Example: this will apply blur to all frames with AverageLuma < 50
```
LoadPlugin("ConditionalMT.dll")
vid = AviSource("Source.avi")
vid_blur = vid.Blur(1.5)
ConditionalFilterMT(vid, vid_blur, vid, "AverageLuma", "lessthan", "50")
Prefetch(4)
```

All the standard function expressions are supported. However, only a single plain function name is supported -- no expression.
Do not include parenthesis.
- AverageLuma
- AverageChromaU
- AverageChromaV
- RGBDifference
- LumaDifference
- ChromaUDifference
- ChromaVDifference
- YDifferenceFromPrevious
- UDifferenceFromPrevious
- VDifferenceFromPrevious
- RGBDifferenceFromPrevious
- YDifferenceToNext
- UDifferenceToNext
- VDifferenceToNext
- RGBDifferenceToNext
- YPlaneMax
- YPlaneMin
- YPlaneMedian
- UPlaneMax
- UPlaneMin
- UPlaneMedian
- VPlaneMax
- VPlaneMin
- VPlaneMedian
- YPlaneMinMaxDifference
- UPlaneMinMaxDifference
- VPlaneMinMaxDifference

Functions comparing two clips will compare testclip with source1. Some functions have threshold and offset parameters. 
These parameters are not currently supported and are left at 0. If you need them, feel free to edit the code to parse parameter values.
