# Frame Rate Converter
# Version: 16-May-2017
# By Etienne Charland
# Based on Oleg Yushko's YFRC artifact masking,
# johnmeyer's frame interpolation code, and
# raffriff42's "weak mask" and output options.
# Special thanks to Pinterf for adding 16-bit support to MvTools2 and MaskTools2
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
# http:#www.gnu.org/copyleft/gpl.html.

#######################################################################################
### Frame Rate Converter
### Increases the frame rate with interpolation and fine artifact removal.
##
## YV12/YV24/Y8/YUY2
## Requires: FrameRateConverter.dll, MaskTools2, MvTools2, GRunT, rgtools (default prefilter)
##
## @ NewNum      - The new framerate numerator (if FrameDouble = false, default = 60)
##
## @ NewDen      - The new framerate denominator (if FrameDouble = false, default = 1)
##
## @ Preset      - The speed/quality preset [slow|normal|fast]. (default=normal)
##
## @ BlkSize     - The block size. Latest MvTools2.dll version from Pinterf supports 6, 8, 12, 16, 24, 32, 48 and 64.
##                 (default = Width>2000||Height>1200 ? 32 : Width>720||C.Height>480 ? 16 : 8)
## 
## @ BlkSizeV    - The vertical block size. (default = BlkSize)
## 
## @ FrameDouble - Whether to double the frame rate and preserve original frames (default = true)
##
## @ Output      - Output mode [auto|flow|none|mask|skip|raw|over] (default = auto)
##                 auto=normal artifact masking; flow=interpolation only; none=ConvertFPS only; mask=mask only; 
##                 skip=mask used to Skip; raw=raw mask; over=mask as cyan overlay for debugging
##
## @ Debug       - Whether to display AverageLuma values of Skip, Mask and Raw. (Default = false)
##
## @ Prefilter   - Specified a custom prefiltered clip. (Default = RemoveGrain(22))
##
## @ MaskTrh     - The treshold where a block is considered bad, between 0 and 255. Smaller = stronger.
##                 0 to disable artifact masking. (Default = 140)
##
## @ MaskOcc     - Occlusion mask treshold, between 0 and 255. 0 to disable occlusion masking. (Default = 105)
##
## @ SkipTrh     - The treshold where a block is counted for the skip mask, between 0 and 255. Smaller = stronger.
##                 Must be smaller (stronger) than MaskTrh. (Default = 60)
##
## @ BlendOver   - Try fallback block size when artifacts cover more than specified treshold, or 0 to disable.
##                 If it fails again, it will revert to frame blending. (default = 50)
##
## @ SkipOver    - Skip interpolation of frames when artifacts cover more than specified treshold, 
##                 or 0 to disable. (Default = 120)
##
function FrameRateConverter(clip C, int "NewNum", int "NewDen", string "Preset", int "BlkSize", int "BlkSizeV", bool "FrameDouble", 
\ string "Output", bool "Debug", clip "Prefilter", int "MaskTrh", int "MaskOcc", int "SkipTrh", int "BlendOver", int "SkipOver")
{
    Preset = Default(Preset, "normal")
    P_SLOW = 0  P_NORMAL = 1  P_FAST = 2
    Pset = Preset == "slow" ? P_SLOW : Preset == "normal" ? P_NORMAL : Preset == "fast" ? P_FAST : -1
    Assert(Pset != -1, "FrameRateConverter: 'Preset' must be slow, normal or fast {'" + Preset + "'}")
    Output = Default(Output, "auto")
    O_AUTO = 0  O_FLOW = 1  O_NONE = 2  O_MASK = 3  O_SKIP = 4  O_RAW = 5  O_OVER = 6
    OPut = Output == "auto" ? O_AUTO : Output == "flow" ? O_FLOW : Output == "none" ? O_NONE : Output == "mask" ? O_MASK :
    \  Output == "skip" ? O_SKIP : Output == "raw" ? O_RAW : Output == "over" ? O_OVER : -1
    Assert(OPut != -1, "FrameRateConverter: 'Output' not one of (auto|flow|none|mask|skip|raw|over) {'" + Output + "'}")
    
    FrameDouble = Default(FrameDouble, Defined(NewNum) ? false : true)
    NewNum      = FrameDouble ? C.FrameRateNumerator * 2 : Default(NewNum, 60)
    NewDen      = FrameDouble ? C.FrameRateDenominator : Default(NewDen, 1)
    BlkSize     = Default(BlkSize, C.Width>2000||C.Height>1200 ? 32 : C.Width>720||C.Height>480 ? 16 : 8)
    BlkSizeV    = Default(BlkSizeV, BlkSize)
    MaskTrh     = Default(MaskTrh, 140)
    SkipTrh     = Default(SkipTrh, 60)
    MaskOcc     = MaskTrh > 0 ? Default(MaskOcc, 105) : 0
    BlendOver   = Default(BlendOver, 50)
    SkipOver    = Default(SkipOver, 120)
    CalcPrefilter = Defined(Prefilter) || Pset != P_FAST
    Prefilter   = Default(Prefilter, CalcPrefilter ? C.RemoveGrain(22) : C)
    Debug       = Default(Debug, false)
    OutFps      = OPut!=O_MASK && OPut!=O_SKIP && OPut!=O_RAW  # Whether output has altered frame rate
    Recalculate = PSET <= P_NORMAL
    DCT         = PSET == P_SLOW ? 1 : 0

    Assert(MaskTrh >= 0 && MaskTrh <= 255, String(MaskTrh, "FrameRateConverter: MaskTrh must be between 0 and 255 {%.f}"))
    Assert(MaskOcc >= 0 && MaskOcc <= 255, String(MaskOcc, "FrameRateConverter: MaskOcc must be between 0 and 255 {%.f}"))
    Assert(SkipTrh < MaskTrh, "FrameRateConverter: SkipTrh must be lower (stronger) than MaskTrh")
    Assert(BlendOver >= 0 && BlendOver <= 255, String(BlendOver, "FrameRateConverter: BlendOver must be between 0 and 255 {%.f}"))
    Assert(SkipOver >= 0 && SkipOver <= 255, String(SkipOver, "FrameRateConverter: SkipOver must be between 0 and 255 {%.f}"))
    Assert(BlendOver==0 || SkipOver==0 || SkipOver > BlendOver, "FrameRateConverter: SkipOver must be greater than BlendOver")

    ## "B" - Blending, "BHard" - No blending
    B = C.ConvertFps(NewNum, NewDen)
    BHard = C.ChangeFps(NewNum, NewDen)

    ## Adjust parameters for different block sizes, causing stronger or weaker masks
    blk = Max(BlkSize, BlkSizeV)
    MaskTrh = MaskTrh + (blk<=4 ? -40 : blk<=6 ? -30 : blk<=8 ? -20 : blk<=12 ? -10 : blk<=16 ? 0 : 
    \         blk<=24 ? 10 : blk<=32 ? 20 : blk<=48 ? 28 : 35)
    SkipTrh = SkipTrh + (blk<=4 ? -33 : blk<=6 ? -26 : blk<=8 ? -18 : blk<=12 ? -9 : blk<=16 ? 0 : 
    \         blk<=24 ? 8 : blk<=32 ? 16 : blk<=48 ? 23 : 30)
    MaskTrh = Max(Min(MaskTrh, 255), 0)
    SkipTrh = Max(Min(SkipTrh, 255), 0)
    gam = blk<=4 ? .60 : blk<=6 ? .58 : blk<=8 ? .56 : blk<=12 ? .54 : blk<=16 ? .50 : 
    \         blk<=24 ? .44 : blk<=32 ? .36 : blk<=48 ? .26 : .14

    ## jm_fps interpolation
    superfilt = MSuper(prefilter, hpad=16, vpad=16) # all levels for MAnalyse
    super = CalcPrefilter ? MSuper(C, hpad=16, vpad=16, levels=1) : superfilt # one level is enough for MRecalculate
    bak = MAnalyse(superfilt, isb=true, blksize=BlkSize, blksizev=BlkSizeV, overlap = BlkSize>4?(BlkSize/4+1)/2*2:0, overlapv = BlkSizeV>4?(BlkSizeV/4+1)/2*2:0, search=3, dct=DCT)
    fwd = MAnalyse(superfilt, isb=false, blksize=BlkSize, blksizev=BlkSizeV, overlap = BlkSize>4?(BlkSize/4+1)/2*2:0, search=3, dct=DCT)
    fwd = Recalculate ? MRecalculate(super, fwd, blksize=BlkSize/2, blksizev=BlkSizeV/2, overlap = BlkSize/2>4?(BlkSize/8+1)/2*2:0, overlapv = BlkSizeV/2>4?(BlkSizeV/8+1)/2*2:0, thSAD=100) : fwd
    bak = Recalculate ? MRecalculate(super, bak, blksize=BlkSize/2, blksizev=BlkSizeV/2, overlap = BlkSize/2>4?(BlkSize/8+1)/2*2:0, overlapv = BlkSizeV/2>4?(BlkSizeV/8+1)/2*2:0, thSAD=100) : bak
    Flow = MFlowFps(C, super, bak, fwd, num=NewNum, den=NewDen, blend=false, ml=200, mask=2, thSCD2=255)

    ## "EM" - error or artifact mask
    # Mask: SAD
    EM = MaskTrh > 0 ? C.ConvertToY8().MMask(bak, ml=255, kind=1, gamma=1/gam, ysc=255, thSCD2=255) : 
    \                  BlankClip(C, pixel_type="Y8", color_yuv=$000000)
    # Mask: Temporal blending
    EMfwd = MaskTrh > 0 ? C.ConvertToY8().MMask(fwd, ml=255, kind=1, gamma=1/gam, thSCD2=255) : EM
    EM = MaskTrh > 0 ? EM.Overlay(EMfwd, opacity=.6, mode="lighten", pc_range=true) : EM
    # Mask: Occlusion
    EMocc = MaskOcc > 0 ? C.ConvertToY8().MMask(bak, ml=MaskOcc, kind=2, gamma=1/gam, ysc=255, thSCD2=255)
    \                     .mt_inpand() : BlankClip(C, pixel_type="Y8", color_yuv=$000000)
    EM = MaskOcc > 0 ? EM.Overlay(EMocc, opacity=.4, mode="lighten", pc_range=true) : EM
    # Mask: Stripes
    EMstp = C.StripeMask(blksize=BlkSize, blksizev=BlkSizeV, str=140, strf=70, trh=22)
    EM = EM.Overlay(EMstp, opacity=.5, mode="lighten", pc_range=true)
    # Last mask frame is white. Replace with previous frame.
    EM = EM.DeleteFrame(EM.Framecount-1).Loop(2, EM.Framecount-1)
    OutRaw = EM
    
    ## Mask processing
    EM = EM.BicubicResize(Round(C.Width/BlkSize/4.0)*4, Round(C.Height/BlkSize/4.0)*4)
    \       .mt_expand(mode= mt_circle(zero=true, radius=1))
    EMskip = EM.mt_binarize(SkipTrh)
    EM = EM.mt_binarize(MaskTrh)
    \       .Blur(.6)
    \       .BicubicResize(C.Width, C.Height)
    OutSkip = EMskip.BicubicResize(C.width, C.Height)
    
    ## "M" - Apply artifact removal
    EM = OutFps ? EM.ChangeFPS(NewNum, NewDen) : EM
    EMskip = OutFps ? EMskip.ChangeFPS(NewNum, NewDen) : EMskip
    M = OutFps ? mt_merge(Flow, B, EM, luma=true, chroma="process") : Flow

    ## Apply BlendOver and SkipOver
    M2 = SkipOver > 0 ? ConditionalFilterMT(EMskip, B, BHard, "AverageLuma", "<", string(SkipOver)) : B
    M = BlendOver > 0 ? ConditionalFilterMT(EMskip, M, M2, "AverageLuma", "<", string(BlendOver)) : M
#    M = M.GScriptClip("Skip = EMskip.AverageLuma()
#    \  (" + string(SkipOver) + " > 0 && Skip >= " + string(SkipOver) + ") ? BHard : 
#    \  (" + string(BlendOver) + " > 0 && Skip >= " + string(BlendOver) + ") ? B : M",
#    \  args = "EMskip,M,B,BHard", Local=true)

    # Output modes
    R= (Oput==O_AUTO)                               [** auto: artifact masking     *]
    \     ? (FrameDouble ? Interleave(C, SelectOdd(M)) : M)
    \ : (Oput==O_FLOW)                              [** flow: interpolation only   *]
    \     ? Flow
    \ : (Oput==O_NONE)                              [** none: ConvertFPS only      *]
    \     ? B
    \ : (Oput==O_MASK)                              [** mask: mask only            *]
    \     ? EM
    \ : (Oput==O_SKIP)                              [** skip: skip mask            *]
    \     ? OutSkip
    \ : (Oput==O_RAW)                               [** raw:  raw mask             *]
    \     ? OutRaw.ColorYUV(levels="PC->TV")
    \ : (Oput==O_OVER)                              [** over: mask as cyan overlay *]
    \     ? Flow.Overlay(MergeRGB(BlankClip(EM, color_yuv=$000000), EM, EM), mode="Add", opacity=0.40, pc_range=true)
    \ : Assert(false, "FrameRateConverter: 'Output' INTERNAL ERROR")

    # Debug: display AverageLuma values of Skip, Mask and Raw
    ShowRaw = OutFps ? OutRaw.ChangeFPS(NewNum, NewDen) : OutRaw
    R = Debug ? R.GScriptClip("""Skip = EMskip.AverageLuma()
    \           SkipSoft = BlendOver > 0 && Skip >= BlendOver && (Skip < SkipOver || SkipOver == 0)
    \           Subtitle("Skip:  " + string(Skip) + "\nMask: " + string(EM.AverageLuma) + 
    \           "\nRaw:  " + string(ShowRaw.AverageLuma) + 
    \           "\nBlkSize: " + string(BlkSize) + 
    \           (SkipSoft ? " - Blend" : "") +
    \           (SkipOver > 0 && Skip >= SkipOver ? " - Skip" : ""), lsp=0)""",
    \           args = "EMskip,EM,ShowRaw,BlkSize,SkipOver,BlendOver", Local=true) : R
    return R
}