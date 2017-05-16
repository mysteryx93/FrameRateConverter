
// Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
// http://www.avisynth.org

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .
//
// Linking Avisynth statically or dynamically with other modules is making a
// combined work based on Avisynth.  Thus, the terms and conditions of the GNU
// General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Avisynth give you
// permission to link Avisynth with independent modules that communicate with
// Avisynth solely through the interfaces defined in avisynth.h, regardless of the license
// terms of these independent modules, and to copy and distribute the
// resulting combined work under terms of your choice, provided that
// every copy of the combined work is accompanied by a complete copy of
// the source code of Avisynth (the version of Avisynth used to produce the
// combined work), being distributed under the terms of the GNU General
// Public License plus this exception.  An independent module is a module
// which is not derived from or based on Avisynth, such as 3rd-party filters,
// import and export plugins, or graphical user interfaces.

#include "conditional.h"

#define W_DIVISOR 5  // Width divisor for onscreen messages

/********************************
 * Conditional filter
 *
 * Returns each one frame from two sources,
 * based on an evaluator.
 ********************************/

ConditionalFilter::ConditionalFilter(PClip _child, PClip _source1, PClip _source2,
	AVSValue  _condition1, AVSValue  _evaluator, AVSValue  _condition2,
	bool _show, IScriptEnvironment* env) :
	GenericVideoFilter(_child), source1(_source1), source2(_source2),
	eval1(_condition1), eval2(_condition2), show(_show) {

	evaluator = NONE;

	if (lstrcmpi(_evaluator.AsString(), "equals") == 0 ||
		lstrcmpi(_evaluator.AsString(), "=") == 0 ||
		lstrcmpi(_evaluator.AsString(), "==") == 0)
		evaluator = EQUALS;
	if (lstrcmpi(_evaluator.AsString(), "greaterthan") == 0 || lstrcmpi(_evaluator.AsString(), ">") == 0)
		evaluator = GREATERTHAN;
	if (lstrcmpi(_evaluator.AsString(), "lessthan") == 0 || lstrcmpi(_evaluator.AsString(), "<") == 0)
		evaluator = LESSTHAN;

	if (evaluator == NONE)
		env->ThrowError("ConditionalFilter: Evaluator could not be recognized!");

	VideoInfo vi1 = source1->GetVideoInfo();
	VideoInfo vi2 = source2->GetVideoInfo();

	if (vi1.height != vi2.height)
		env->ThrowError("ConditionalFilter: The two sources must have the same height!");
	if (vi1.width != vi2.width)
		env->ThrowError("ConditionalFilter: The two sources must have the same width!");
	if (!vi1.IsSameColorspace(vi2))
		env->ThrowError("ConditionalFilter: The two sources must be the same colorspace!");

	vi.height = vi1.height;
	vi.width = vi1.width;
	vi.pixel_type = vi1.pixel_type;
	vi.num_frames = max(vi1.num_frames, vi2.num_frames);
	vi.num_audio_samples = vi1.num_audio_samples;
	vi.audio_samples_per_second = vi1.audio_samples_per_second;
	vi.image_type = vi1.image_type;
	vi.fps_denominator = vi1.fps_denominator;
	vi.fps_numerator = vi1.fps_numerator;
	vi.nchannels = vi1.nchannels;
	vi.sample_type = vi1.sample_type;
}

const char* const t_TRUE = "TRUE";
const char* const t_FALSE = "FALSE";


PVideoFrame __stdcall ConditionalFilter::GetFrame(int n, IScriptEnvironment* env) {

	VideoInfo vi1 = source1->GetVideoInfo();
	VideoInfo vi2 = source2->GetVideoInfo();

	AVSValue e1_result;
	AVSValue e2_result;
	try {
		e1_result = ConditionalFunction(eval1.AsString(), child, source1, n, env);
		e2_result = ConditionalFunction(eval2.AsString(), child, source1, n, env);
	}
	catch (const AvisynthError &error) {
		const char* error_msg = error.msg;

		PVideoFrame dst = source1->GetFrame(n, env);
		env->MakeWritable(&dst);
		env->ApplyMessage(&dst, vi1, error_msg, vi.width / W_DIVISOR, 0xa0a0a0, 0, 0);
		return dst;
	}

	bool test_int = false;
	bool test_string = false;

	int e1 = 0;
	int e2 = 0;
	float f1 = 0.0f;
	float f2 = 0.0f;
	try {
		if (e1_result.IsString()) {
			if (!e2_result.IsString())
				env->ThrowError("Conditional filter: Second expression did not return a string, as in first string expression.");
			test_string = true;
			test_int = true;
			e1 = lstrcmp(e1_result.AsString(), e2_result.AsString());
			e2 = 0;

		}
		else if (e1_result.IsBool()) {
			if (!(e2_result.IsInt() || e2_result.IsBool()))
				env->ThrowError("Conditional filter: Second expression did not return an integer or bool, as in first bool expression.");
			test_int = true;
			e1 = e1_result.AsBool();
			e2 = e2_result.IsInt() ? e2_result.AsInt() : e2_result.AsBool();

		}
		else if (e1_result.IsInt()) {
			if (e2_result.IsInt() || e2_result.IsBool()) {
				test_int = true;
				e1 = e1_result.AsInt();
				e2 = e2_result.IsInt() ? e2_result.AsInt() : e2_result.AsBool();
			}
			else if (e2_result.IsFloat()) {
				f1 = (float)e1_result.AsFloat();
				f2 = (float)e2_result.AsFloat();
			}
			else
				env->ThrowError("Conditional filter: Second expression did not return a float, integer or bool, as in first integer expression.");

		}
		else if (e1_result.IsFloat()) {
			f1 = (float)e1_result.AsFloat();
			if (!e2_result.IsFloat())
				env->ThrowError("Conditional filter: Second expression did not return a float or an integer, as in first float expression.");
			f2 = (float)e2_result.AsFloat();
		}
		else {
			env->ThrowError("ConditionalFilter: First expression did not return an integer, bool or float!");
		}
	}
	catch (const AvisynthError &error) {
		const char* error_msg = error.msg;

		PVideoFrame dst = source1->GetFrame(n, env);
		env->MakeWritable(&dst);
		env->ApplyMessage(&dst, vi1, error_msg, vi.width / W_DIVISOR, 0xa0a0a0, 0, 0);
		return dst;
	}


	bool state = false;

	if (test_int) { // String and Int compare
		if (evaluator&EQUALS)
			if (e1 == e2) state = true;

		if (evaluator&GREATERTHAN)
			if (e1 > e2) state = true;

		if (evaluator&LESSTHAN)
			if (e1 < e2) state = true;

	}
	else {  // Float compare
		if (evaluator&EQUALS)
			if (fabs(f1 - f2) < 0.000001f) state = true;   // Exact equal will sometimes be rounded to wrong values.

		if (evaluator&GREATERTHAN)
			if (f1 > f2) state = true;

		if (evaluator&LESSTHAN)
			if (f1 < f2) state = true;
	}

	if (show) {
		char text[400];
		if (test_string) {
			_snprintf(text, sizeof(text) - 1,
				"Left side Conditional Result:%.40s\n"
				"Right side Conditional Result:%.40s\n"
				"Evaluate result: %s\n",
				e1_result.AsString(), e2_result.AsString(), (state) ? t_TRUE : t_FALSE
			);
		}
		else if (test_int) {
			_snprintf(text, sizeof(text) - 1,
				"Left side Conditional Result:%i\n"
				"Right side Conditional Result:%i\n"
				"Evaluate result: %s\n",
				e1, e2, (state) ? t_TRUE : t_FALSE
			);
		}
		else {
			_snprintf(text, sizeof(text) - 1,
				"Left side Conditional Result:%7.4f\n"
				"Right side Conditional Result:%7.4f\n"
				"Evaluate result: %s\n",
				f1, f2, (state) ? t_TRUE : t_FALSE
			);
		}

		PVideoFrame dst = (state) ? source1->GetFrame(min(vi1.num_frames - 1, n), env) : source2->GetFrame(min(vi2.num_frames - 1, n), env);
		env->MakeWritable(&dst);
		env->ApplyMessage(&dst, vi, text, vi.width / 4, 0xa0a0a0, 0, 0);

		return dst;
	}

	if (state)
		return source1->GetFrame(min(vi1.num_frames - 1, n), env);

	return source2->GetFrame(min(vi1.num_frames - 1, n), env);
}

void __stdcall ConditionalFilter::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
	source1->GetAudio(buf, start, count, env);
}

int __stdcall ConditionalFilter::SetCacheHints(int cachehints, int frame_range) {
	return cachehints == CachePolicyHint::CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
}