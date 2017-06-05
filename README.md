# FrameRateConverter
Increases the frame rate with interpolation and fine artifact removal.

by Etienne Charland

This project contains
- FrameRateConverter (AVSI)
- ConditionalFilterMT (DLL)
- StripeMask (DLL)


## FrameRateConverter

Increases the frame rate with interpolation and fine artifact removal.

FrameRateConverter(C, NewNum, NewDen, Preset, BlkSize, BlkSizeV, FrameDouble, Output, Debug, Prefilter, MaskTrh, MaskOcc, SkipTrh, BlendOver, SkipOver)

YV12/YV24/Y8/YUY2  
Requires: FrameRateConverter.dll, MaskTools2, MvTools2 (pinterf), GRunT, rgtools (default prefilter)

@ NewNum      - The new framerate numerator (if FrameDouble = false, default = 60)

@ NewDen      - The new framerate denominator (if FrameDouble = false, default = 1)

@ Preset      - The speed/quality preset [slower|slow|normal|fast]. (default=normal)

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
                auto=normal artifact masking; flow=interpolation only; over=mask as cyan overlay; none=ConvertFPS only; raw=raw mask; 
                mask=mask only; skip=mask used to Skip; diff=mask where alternate interpolation is better; stripe=mask used to cover stripes

@ Debug       - Whether to display AverageLuma values of Skip, Mask and Raw. (Default = false)

@ Prefilter   - Specified a custom prefiltered clip. (Default = RemoveGrain(22))

@ MaskTrh     - The treshold where a block is considered bad, between 0 and 255. Smaller = stronger.
                0 to disable artifact masking. (Default = 145)

@ MaskOcc     - Occlusion mask treshold, between 0 and 255. 0 to disable occlusion masking. (Default = 105)

@ SkipTrh     - The treshold where a block is counted for the skip mask, between 0 and 255. Smaller = stronger.
                Must be smaller (stronger) than MaskTrh. (Default = 60)

@ BlendOver   - Try fallback block size when artifacts cover more than specified treshold, or 0 to disable.
                If it fails again, it will revert to frame blending. (default = 50)

@ SkipOver    - Skip interpolation of frames when artifacts cover more than specified treshold, 
                or 0 to disable. (Default = 120)

@ DiffBlkSize - If specified, calculates a second interpolation with different settings to takes the areas where the mask looks better. (Default = 0)
                
@ DiffBlkSizeV- Vertical block size for second interpolation clip (default = DiffBlkSize)

@ Stripes     - How to deal with stripes [none|skip|blend] (default=skip)


Presets  
Fast:    Basic interpolation  
Normal:  Fast + prefilter + MSuper on prefilter + MRecalculate  
Slow:    Normal + DCT=1  
Slower:  Slow + calculate diff between DCT=1 and DCT=0 to take the best from both  




## StripeMask

Builds a mask detecting horizontal and vertical straight lines and patterns, as MvTools tends to fail in such areas.

StripeMask(C, BlkSize, BlkSizeV, Overlap, OverlapV, Trh, Comp, CompV, Str, StrF, Lines

@ BlkSize, BlkSizeV     - The horizontal and vertical block size. (default: BlkSize=16, BlkSizeV=BlkSize)

@ Overlap, OverlapV     - How many pixels to overlap between blocks, generally between 1/4 and 1/2 of block size. (default = BlkSize/4)

@ Trh                   - Dynamic content gives blended (grey) line averages while lines and stripes have contrast between average values. This specifies the contrast threshold where a line average is taken for calculations. A lower value gives a stronger and more sensitive mask. (default = 26)

@ Comp, CompV           - How many lines averages to compare with each other. (default = 2 with BlkSize<16 and 3 with BlkSize>=16)

@ Str                   - The value to set on the mask when a pattern is detected, between 0 and 255. (default = 255)

@ StrF                  - If > 0, calculates the next frame and set its patterns to this value, between 0 and 255. (default = 0)

@ Lines                 - If true, display the raw contrast lines being used for calculations. If false, the pattern areas between those lines will be marked. (default = false)



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

Functions comparing two clips will compare testclip with source1. Some functions have treshold and offset parameters. 
These parameters are not currently supported and are left at 0. If you need them, feel free to edit the code to parse parameter values.
