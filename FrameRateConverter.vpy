# Frame Rate Converter
# Version: 2.0 (2021-06-21)
# By Etienne Charland
# Based on Oleg Yushko's YFRC artifact masking,
# johnmeyer's frame interpolation code, and
# raffriff42's "weak mask" and output options.
# Pinterf is the one who spent the most time working on the core libraries, adding features and fixing bugs
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
## Requires: FrameRateConverter.dll, MaskTools2, MvTools2 (pinterf), GRunT (for debug only)
##
## @ newNum      - The new framerate numerator (if frameDouble = False, default = 60)
##
## @ newDen      - The new framerate denominator (if frameDouble = False, default = 1)
##
## @ preset      - The speed/quality preset [slowest|slower|slow|normal|fast|faster|anime]. (default=normal)
##
## @ blkSize     - The block size. Latest MvTools2.dll version from Pinterf supports 6, 8, 12, 16, 24, 32, 48 and 64.
##                 Defaults for 4/3 video of height:
##                 0-359:  8
##                 360-749: 12
##                 750-1199: 16
##                 1200-1699: 24
##                 1600-2160: 32
## 
## @ blkSizeV    - The vertical block size. (default = blkSize)
## 
## @ frameDouble - Whether to double the frame rate and preserve original frames (default = True)
##
## @ output      - output mode [auto|flow|over|none|raw|mask|skip|diff|stripe] (default = auto)
##                 auto=normal artifact masking; flow=interpolation only; over=mask as cyan overlay, stripes mask as yellow; none=ConvertFPS only; raw=raw mask; 
##                 mask=mask only; skip=mask used to Skip; diff=mask where alternate interpolation is better; stripe=mask used to cover stripes
##
## @ debug       - Whether to display AverageLuma values of Skip, Mask and Raw. (Default = False)
##
## @ prefilter   - Specifies a prefilter such as RgTools' RemoveGrain(21). Recommended only when not using a denoiser (Default=none)
##
## @ maskThr     - The threshold where a block is considered bad, between 0 and 255. Smaller = stronger.
##                 0 to disable artifact masking. (Default = 120)
##
## @ maskOcc     - Occlusion mask threshold, between 0 and 255. 0 to disable occlusion masking. (Default = 105)
##
## @ skipThr     - The threshold where a block is counted for the skip mask, between 0 and 255. Smaller = stronger.
##                 Must be smaller (stronger) than maskThr. (Default = 55)
##
## @ blendOver   - Try fallback block size when artifacts cover more than specified threshold, or 0 to disable.
##                 If it fails again, it will revert to frame blending. (default = 70)
##
## @ skipOver    - Skip interpolation of frames when artifacts cover more than specified threshold, 
##                 or 0 to disable. (Default = 210)
##                 
## @ stp         - Whether to detect and blend stripes (default=True)
##
## @ dct         - Overrides DCT parameter for MAnalyse (default: Normal=0, Slow=4, Slowest=1)
##                 
## @ dctRe       - Overrides DCT parameter for MRecalculate (default: Fast=0, Normal=4, Slowest=1)
##
## @ blendRatio  - Changes the blend ratio used to fill artifact zones. 0 = frame copy and 100 = full blend.
##                 Other values provide a result in-between to eliminate ghost effects. Default = 40.
##                 
##
## Presets
## Faster:  Basic interpolation
## Fast:    MRecalculate
## Normal:  MRecalculate with DCT=4
## Slow:    MAnalyze + MRecalculate with DCT=4
## Slower:  Calculate diff between DCT=4 and DCT=0 to take the best from both
## Slowest: Calculate diff between DCT=1 and DCT=0 to take the best from both
## Anime:   Slow with blendOver=40, skipOver=140
##

import functools
import havsfunc as havs
import vapoursynth as vs
core = vs.get_core()

def FrameRateConverter(C, newNum = None, newDen = None, preset = "normal", blkSize = None, blkSizeV = None, frameDouble = None, output = "auto", debug = False, \
    prefilter = None, maskThr = 120, maskOcc = None, skipThr = 55, blendOver = None, skipOver = None, stp = True, dct = None, dctRe = None, blendRatio = 40):
    if not isinstance(C, vs.VideoNode):
        raise vs.Error('FrameRateConverter: This is not a clip')

    P_SLOWEST, P_SLOWER, P_SLOW, P_NORMAL, P_FAST, P_FASTER, P_ANIME = 0, 1, 2, 3, 4, 5, 6
    pset = P_SLOWEST if preset == "slowest" else P_SLOWER if preset == "slower" else P_SLOW if preset == "slow" else P_NORMAL if preset == "normal" else P_FAST if preset == "fast" else P_FASTER if preset == "faster" else P_ANIME if preset == "anime" else -1
    if (pset < 0):
        raise vs.Error("FrameRateConverter: 'preset' must be slowest, slower, slow, normal, fast, faster or anime {'" + preset + "'}")
    O_AUTO, O_FLOW, O_OVER, O_NONE, O_RAW, O_MASK, O_SKIP, O_DIFF, O_STRIPE = 0, 1, 2, 3, 4, 5, 6, 7, 8
    oput = O_AUTO if output == "auto" else O_FLOW if output == "flow" else O_OVER if output == "over" else O_NONE if output == "none" else O_RAW if output == "raw" else O_MASK if output == "mask" else O_SKIP if output == "skip" else O_DIFF if output == "diff" else O_STRIPE if output == "stripe" else -1
    if (oput < 0):
        raise vs.Error("FrameRateConverter: 'output' not one of (auto|flow|none|mask|skip|raw|diff|over) {'" + output + "'}")
    
    frameDouble = bool(frameDouble) or not bool(newNum)
    newNum      = C.fps_num * 2 if frameDouble else (newNum or 60)
    newDen      = C.fps_den if frameDouble else (newDen or 1)
    if newNum == C.fps_num * 2 and newDen == C.fps_den:
        frameDouble = True
    defH        = max(C.height, C.width//4*3)
    blkSize     = blkSize or (8 if defH<360 else 12 if defH<750 else 16 if defH<1200 else 24 if defH<1600 else 32)
    blkSizeV    = blkSizeV or blkSize
    maskOcc     = (maskOcc or 105) if maskThr > 0 else 0
    blendOver   = blendOver or (40 if pset==P_ANIME else 70)
    skipOver    = skipOver or (140 if pset==P_ANIME else 210)
    calcPrefilter = bool(prefilter)
    prefilter   = prefilter or C

    outFps      = oput!=O_MASK and oput!=O_SKIP and oput!=O_RAW and oput!=O_DIFF and oput!=O_STRIPE  # Whether output has altered frame rate
    if pset == P_ANIME:
        pset = P_SLOW
    recalculate = pset <= P_FAST
    dctRe       = (dctRe or dct or (1 if pset<=P_SLOWEST else 4 if pset<=P_NORMAL else 0)) if recalculate else 0
    dct         = dct or 1 if pset<=P_SLOWEST else 4 if pset<=P_SLOW else 0
    calcDiff    = pset <= P_SLOWER

    if maskThr < 0 or maskThr > 255:
        raise vs.Error(f"FrameRateConverter: maskThr must be between 0 and 255 {maskThr:n}")
    if maskOcc < 0 or maskOcc > 255:
        raise vs.Error(f"FrameRateConverter: maskOcc must be between 0 and 255 {maskOcc:n}")
    if skipThr >= maskThr:
        raise vs.Error("FrameRateConverter: skipThr must be lower (stronger) than maskThr")
    if blendOver < 0 or blendOver > 255:
        raise vs.Error(f"FrameRateConverter: blendOver must be between 0 and 255 {blendOver:n}")
    if skipOver < 0 or skipOver > 255:
        raise vs.Error(f"FrameRateConverter: skipOver must be between 0 and 255 {skipOver:n}")
    if blendOver and skipOver and (skipOver <= blendOver):
        raise vs.Error("FrameRateConverter: skipOver must be greater than blendOver")
    if oput == O_DIFF and not calcDiff:
        raise vs.Error("FrameRateConverter: You can only use output='Diff' when using preset=slower or slowest")

    ## "B" - Blending, "BHard" - No blending
    B = C.frc.ConvertFpsLimit(newNum, newDen, ratio=blendRatio)
    BHard = havs.ChangeFPS(C, newNum, newDen)
    Blank = core.std.BlankClip(C.resize.Point(format=vs.GRAY8))

    ## Adjust parameters for different block sizes, causing stronger or weaker masks
    blk = max(blkSize, blkSizeV)
    maskThr += -40 if blk<=4 else -30 if blk<=6 else -20 if blk<=8 else -10 if blk<=12 else 0 if blk<=16 else 10 if blk<=24 else 20 if blk<=32 else 28 if blk<=48 else 35
    skipThr += -33 if blk<=4 else -26 if blk<=6 else -18 if blk<=8 else -9 if blk<=12 else 0 if blk<=16 else 8 if blk<=24 else 16 if blk<=32 else 23 if blk<=48 else 30
    maskThr = max(min(maskThr, 255), 0)
    skipThr = max(min(skipThr, 255), 0)
    
    gam = .60 if blk<=4 else .58 if blk<=6 else .56 if blk<=8 else .54 if blk<=12 else .50 if blk<=16 else .44 if blk<=24 else .36 if blk<=32 else .26 if blk<=48 else .14
    dct_mult = .9 if not recalculate else 1.1 if dctRe==2 else 1.15 if dctRe==3 else 1.1 if dctRe==4 else 1.35 if dctRe==1 else 1
    dct_pow = 1 if not recalculate else 1.01 if dctRe==2 else 1.0 if dctRe==3 else 1.073 if dctRe==4 else 1.09 if dctRe==1 else 1

    ## jm_fps interpolation
    superfilt = core.mv.Super(prefilter, hpad=16, vpad=16, sharp=1, rfilter=4) # all levels for MAnalyse
    super = core.mv.Super(C, hpad=16, vpad=16, levels=1, sharp=1, rfilter=4) if calcPrefilter else superfilt # one level is enough for MRecalculate
    bak = bak2 = core.mv.Analyse(superfilt, isb=True, blksize=blkSize, blksizev=blkSizeV, overlap = (blkSize//4+1)//2*2 if blkSize>4 else 0, overlapv = (blkSizeV//4+1)//2*2 if blkSizeV>4 else 0, search=3, dct=dct)
    fwd = fwd2 = core.mv.Analyse(superfilt, isb=False, blksize=blkSize, blksizev=blkSizeV, overlap = (blkSize//4+1)//2*2 if blkSize>4 else 0, overlapv = (blkSizeV//4+1)//2*2 if blkSizeV>4 else 0, search=3, dct=dct)
    if recalculate:
        fwd = core.mv.Recalculate(super, fwd, blksize=blkSize//2, blksizev=blkSizeV//2, overlap = (blkSize//8+1)//2*2 if blkSize/2>4 else 0, overlapv = (blkSizeV/8+1)//2*2 if blkSizeV/2>4 else 0, thsad=100, dct=dctRe)
        bak = core.mv.Recalculate(super, bak, blksize=blkSize//2, blksizev=blkSizeV//2, overlap = (blkSize//8+1)//2*2 if blkSize/2>4 else 0, overlapv = (blkSizeV/8+1)//2*2 if blkSizeV/2>4 else 0, thsad=100, dct=dctRe)
    Flow = core.mv.FlowFPS(C, super, bak, fwd, num=newNum, den=newDen, blend=False, ml=200, mask=2, thscd2=255)

    ## "EM" - error or artifact mask
    EM = EMfwd = EMooc = EM = Blank
    # Mask: SAD
    if maskThr > 0:
        EM = ToGray(C.mv.Mask(bak, ml=255, kind=1, gamma=1/gam, ysc=255, thscd2=skipOver))
        # Mask: Temporal blending
        EMfwd = ToGray(C.mv.Mask(fwd, ml=255, kind=1, gamma=1/gam, thscd2=skipOver))
        EM = havs.Overlay(EM, EMfwd, opacity=.6, mode="lighten")
    #return EM.resize.Point(range_in=1, range=0)
    # Mask: Occlusion
    if maskOcc > 0:
        EMocc = ToGray(C.mv.Mask(bak, ml=maskOcc, kind=2, gamma=1/gam, ysc=255, thscd2=skipOver).std.Minimum())
        EM = havs.Overlay(EM, EMocc, opacity=.4, mode="lighten")
    if dct_mult!=1 or dct_pow!=1:
       EM = core.std.Expr(EM, f"x {dct_mult} * {dct_pow} pow")

    ## For calcDiff, calculate a 2nd version and create mask to restore from 2nd version the areas that look better
    if calcDiff:
        EM2 = EMfwd2 = EMooc2 = EM2 = Blank
        bakA = core.mv.Analyse(superfilt, isb=True, blksize=blkSize, blksizev=blkSizeV, overlap = (blkSize//4+1)//2*2 if blkSize>4 else 0, overlapv = (blkSizeV//4+1)//2*2 if blkSizeV>4 else 0, search=3, dct=0)
        fwdA = core.mv.Analyse(superfilt, isb=False, blksize=blkSize, blksizev=blkSizeV, overlap = (blkSize//4+1)//2*2 if blkSize>4 else 0, overlapv = (blkSizeV//4+1)//2*2 if blkSizeV>4 else 0, search=3, dct=0)
        if recalculate:
            fwd2 = core.mv.Recalculate(super, fwdA, blksize=blkSize//2, blksizev=blkSizeV//2, overlap = (blkSize//8+1)//2*2 if blkSize//2>4 else 0, overlapv = (blkSizeV//8+1)//2*2 if blkSizeV//2>4 else 0, thsad=100)
            bak2 = core.mv.Recalculate(super, bakA, blksize=blkSize//2, blksizev=blkSizeV//2, overlap = (blkSize//8+1)//2*2 if blkSize//2>4 else 0, overlapv = (blkSizeV//8+1)//2*2 if blkSizeV//2>4 else 0, thsad=100)
        Flow2 = core.mv.FlowFPS(C, super, bak2, fwd2, num=newNum, den=newDen, blend=False, ml=200, mask=2, thscd2=255)

        # Get raw mask again
        if maskThr > 0:
            EM2 = ToGray(C.mv.Mask(bak2, ml=255, kind=1, gamma=1/gam, ysc=255, thscd2=skipOver))
            EMfwd2 = ToGray(C.mv.Mask(fwd2, ml=255, kind=1, gamma=1/gam, thscd2=skipOver))
            EM2 = havs.Overlay(EM2, EMfwd2, opacity=.6, mode="lighten")
        if maskOcc > 0:
            EMocc2 = ToGray(C.mv.Mask(bak2, ml=maskOcc, kind=2, gamma=1/gam, ysc=255, thscd2=skipOver).std.Minimum())
            EM2 = havs.Overlay(EM2, EMocc2, opacity=.4, mode="lighten")

        # Get difference mask between two versions
        EMdiff = core.std.Expr([EM, EM2], "x y -") \
            .resize.Bicubic(round(C.width//blkSize)*4, round(C.height//blkSizeV)*4) \
            .std.Maximum() \
            .std.Binarize(60)
            #.mt_expand(mode= mt_circle(zero=True, radius=2)) \
        EMdiff = frc_GaussianBlur42(EMdiff, 1.2) \
            .resize.Bicubic(C.width, C.height)
        # Apply mask to Flow / EM
        if outFps:
            EMdiff = havs.ChangeFPS(EMdiff, newNum, newDen)
        Flow = core.std.MaskedMerge(Flow, Flow2, EMdiff)
        EM = core.std.MaskedMerge(EM, EM2, EMdiff)

    # Last mask frame is white. Replace with previous frame.
    EM = EM.std.DeleteFrames([EM.num_frames-1]).std.DuplicateFrames([EM.num_frames-2])

    # Create skip mask
    EMskip = EM.resize.Bicubic(round(C.width//blkSize//4.0)*4, round(C.height//blkSizeV//4.0)*4) \
            .std.Maximum() \
            .std.Binarize(skipThr)
            #.mt_expand(mode= mt_circle(zero=True, radius=1)) \
    OutSkip = EMskip.resize.Bicubic(C.width, C.height)
    
    ## Create artifact correction mask
    OutRaw = EM
    EM = EM.resize.Bicubic(round(C.width/blkSize/4.0)*4, round(C.height/blkSizeV/4.0)*4)
    EM = EM.std.Maximum()
    EM = EM.std.Binarize(maskThr)
    #EM = havs.MinBlur(EM, .6)
    EM = EM.resize.Bicubic(C.width, C.height)
    #.std.BoxBlur(hradius=.6, vradius=.6) \
    #.mt_expand(mode= mt_circle(zero=True, radius=1)) \

    # Mask: Stripes
    EMstp = C.frc.StripeMask(blksize=blkSize, blksizev=blkSizeV, str=min(skipThr*2+20, 255), strf=min(skipThr+10, 255), thr=23) \
           .resize.Bicubic(round(C.width//blkSize)*4, round(C.height//blkSizeV)*4) \
           .frc.ContinuousMask(22)
    EMstp = EMstp.resize.Bicubic(EMstp.width//2, EMstp.height//2) \
           .std.Binarize(82) \
           .std.Minimum() \
           .std.Maximum()
           #.mt_expand(mode= mt_circle(zero=True, radius=8))
    EMstp = frc_GaussianBlur42(EMstp, 2.8) \
           .resize.Bicubic(C.width, C.height)
    
    ## "M" - Apply artifact removal
    if outFps:
        EM = havs.ChangeFPS(EM, newNum, newDen)
        EMskip = havs.ChangeFPS(EMskip, newNum, newDen)
        EMstp = havs.ChangeFPS(EMstp, newNum, newDen)
        M = core.std.MaskedMerge(Flow, B, EM)
        if stp:
            M = core.std.MaskedMerge(M, B, EMstp)
    else:
        M = Flow

    def thrSelect(n, f, thr, clipa, clipb, core):
        luma = f.props["PlaneStatsAverage"]
        return clipa if luma < thr else clipb

    ## Apply blendOver and skipOver
    EMskip = EMskip.std.PlaneStats(plane=0)
    M2 = core.std.FrameEval(B, functools.partial(thrSelect, thr=skipOver, clipa=B, clipb=BHard, core=core), prop_src=[EMskip]) if skipOver > 0 else B
    if blendOver > 0:
       M = core.std.FrameEval(M, functools.partial(thrSelect, thr=blendOver, clipa=M, clipb=M2, core=core), prop_src=[EMskip])

    # Prepare output=Over: Mask(cyan), Stripes(yellow)
    Overlays = core.std.ShufflePlanes(clips=[Blank, EM, EM], planes=[0, 0, 0], colorfamily=vs.RGB).resize.Point(format=B.format, matrix_s="709")
    FlowOver = havs.Overlay(Flow, Overlays, mode="addition", opacity=0.40)
    if stp:
        Overlays = core.std.ShufflePlanes(clips=[EMstp, EMstp, Blank], planes=[0, 0, 0], colorfamily=vs.RGB).resize.Point(format=B.format, matrix_s="709")
        FlowOver = havs.Overlay(FlowOver, Overlays, mode="addition", opacity=0.40)

    # output modes
    if oput == O_AUTO:                              # auto: artifact masking
        if frameDouble:
            R = core.std.Interleave([C, M.std.SelectEvery(cycle=2, offsets=1)])
        else:
            R = M
    elif oput == O_FLOW:                            # flow: interpolation only
        R = Flow
    elif oput == O_OVER:                            # over: mask as cyan overlay
        R = FlowOver
    elif oput == O_NONE:                            # none: ConvertFPS only
        R = B
    elif oput == O_RAW:                             # raw:  raw mask
        R = OutRaw.resize.Point(range=1)
    elif oput == O_MASK:                            # mask: mask only
        R = EM
    elif oput == O_SKIP:                            # skip: skip mask
        R = OutSkip
    elif oput == O_DIFF:                            # diff: diff mask
        R = EMdiff
    elif oput == O_STRIPE:                          # stripe: stripes mask
        R = EMstp
    else:
        raise vs.Error("FrameRateConverter: 'output' INTERNAL ERROR")

    def getLuma(f):
        return f.props["PlaneStatsAverage"] * 256
    def setDebugOutput(n, f, R):
        Skip = getLuma(f[0])
        SkipSoft = blendOver > 0 and Skip >= blendOver and (Skip < skipOver or skipOver == 0)
        txt = f"blkSize: {blkSize}"
        if SkipSoft:
            txt += " - Blend"
        if skipOver > 0 and Skip >= skipOver:
            txt += " - Skip"
        txt += f"\nSkip: {Skip:.4f}"
        txt += f"\nRaw: {getLuma(f[1]):.4f}"
        txt += f"\nMask: {getLuma(f[2]):.4f}"
        if calcDiff:
            txt += f"\nDiff: {getLuma(f[3]):.4f}"
        if stp:
            txt += f"\nStripes: {getLuma(f[4]):.4f}"
        return R.text.Text(txt)
    
    # debug: display AverageLuma values of Skip, Mask and Raw
    if 1 or debug:
        ShowRaw = havs.ChangeFPS(OutRaw, newNum, newDen) if outFps else OutRaw
        EMskipLuma = EMskip.std.PlaneStats(plane=0)
        RawLuma = ShowRaw.std.PlaneStats(plane=0)
        EMLuma = EM.std.PlaneStats(plane=0)
        EMdiffLuma = EM.std.PlaneStats(plane=0)
        EMstpLuma = EMstp.std.PlaneStats(plane=0)
        R = R.std.FrameEval(functools.partial(setDebugOutput, R=R), prop_src=[EMskipLuma, RawLuma, EMLuma, EMdiffLuma, EMstpLuma])
    return R


def ToGray(C):
    return core.std.ShufflePlanes(clips=C, planes=0, colorfamily=vs.GRAY)


#######################################################################
### Emulate [[VariableBlur/GaussianBlur]] 
##  For YUV, effective chroma blur varies depending on source 
##  color subsampling - YUV444 has *more* chroma blur, others less.
##
## @ var - works like GaussianBlur's varY
## @ rad - blur radius (<var> squared); overrides <var>
## @ vvar, vrad - vertical var & rad; default same as horizontal
## @ p  - final [[GaussResize]] sharpness. Default 19
##        (if > 25, blockiness; if < 15, loss of contrast)
##
## version 2013-10-23 raffriff42 
## version 2014-05-31 discrete hor. and vert. args
## version 2017-05-21 bugfix: blockiness
## version 2021-06-23 converted to VapourSynth
##
def frc_GaussianBlur42(C, var = None, rad = None, vvar = None, vrad = None, p = None):
    if not isinstance(C, vs.VideoNode):
        raise vs.Error('frc_GaussianBlur42: This is not a clip')

    var = max(0.0, float(var or 1.0))
    rad = max(1.0, float(rad or pow(var, 0.5)))
    var = pow(min(max(0.0, rad), 60.0), 1.9)
	## arbitrary max radius = 60

    vvar = max(0.0, float(vvar or var))
    vrad = max(1.0, float(vrad or pow(vvar, 0.5)))
    vvar = pow(min(max(0.0, vrad), 60.0), 1.9)
    p    = p or 19

    w0 = C.width
    h0 = C.height
    w1 = round(w0/rad)
    h1 = round(h0/vrad) 

    B = C.resize.Bilinear( \
        min(max(4, w1 + (w1 % 2)), w0), \
        min(max(4, h1 + (h1 % 2)), h0))

    # Equivalent to Avisynth Blur(1.0)
    B = B.std.Convolution(matrix=[1, 2, 1, 2, 4, 2, 1, 2, 1]).std.Convolution(matrix=[1, 2, 1, 2, 4, 2, 1, 2, 1])
    
    if var < 0.01 and vvar < 0.01:
        return C
    elif B.width > 8 and B.height > 8:
        return B.fmtc.resample(w0, h0, kernel="gauss", a1=[p]).resize.Point(format=C.format)
    else:
        return B.resize.Bilinear(w0, h0)


core.std.LoadPlugin(path=r'C:\GitHub\FrameRateConverter\Src\x64\debug\FrameRateConverter.dll')
video = core.ffms2.Source(source=r'E:\AVSMeter\Motion Estimation Torture Clip.avi').resize.Point(format=vs.YUV444P8)
video = FrameRateConverter(video, 60, preset="normal", blkSize=16, output="mask", debug=True)
video.set_output()
