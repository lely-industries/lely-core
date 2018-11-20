/**@file
 * This header file is part of the utilities library; it contains the
 * preprocessor definitions.
 *
 * @copyright 2017-2018 Lely Industries N.V.
 *
 * @author J. S. Seldenthuis <jseldenthuis@lely.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LELY_UTIL_CPP_H
#define LELY_UTIL_CPP_H

#include <lely/util/util.h>

#if __STDC_VERSION__ >= 199901L || __cplusplus >= 201103L

/**
 * Repeats all but the first argument <b>n</b> times. <b>n</b> can be at most
 * #CPP_MAX_SIZE.
 */
#define CPP_SEQUENCE(n, ...) \
	CPP_SEQUENCE_(n, __VA_ARGS__)
#define CPP_SEQUENCE_(n, ...) \
	CPP_SEQUENCE_##n(__VA_ARGS__)
#define CPP_SEQUENCE_0(...)
#define CPP_SEQUENCE_1(...) \
	CPP_SEQUENCE_0(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_2(...) \
	CPP_SEQUENCE_1(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_3(...) \
	CPP_SEQUENCE_2(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_4(...) \
	CPP_SEQUENCE_3(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_5(...) \
	CPP_SEQUENCE_4(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_6(...) \
	CPP_SEQUENCE_5(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_7(...) \
	CPP_SEQUENCE_6(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_8(...) \
	CPP_SEQUENCE_7(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_9(...) \
	CPP_SEQUENCE_8(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_10(...) \
	CPP_SEQUENCE_9(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_11(...) \
	CPP_SEQUENCE_10(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_12(...) \
	CPP_SEQUENCE_11(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_13(...) \
	CPP_SEQUENCE_12(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_14(...) \
	CPP_SEQUENCE_13(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_15(...) \
	CPP_SEQUENCE_14(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_16(...) \
	CPP_SEQUENCE_15(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_17(...) \
	CPP_SEQUENCE_16(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_18(...) \
	CPP_SEQUENCE_17(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_19(...) \
	CPP_SEQUENCE_18(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_20(...) \
	CPP_SEQUENCE_19(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_21(...) \
	CPP_SEQUENCE_20(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_22(...) \
	CPP_SEQUENCE_21(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_23(...) \
	CPP_SEQUENCE_22(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_24(...) \
	CPP_SEQUENCE_23(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_25(...) \
	CPP_SEQUENCE_24(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_26(...) \
	CPP_SEQUENCE_25(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_27(...) \
	CPP_SEQUENCE_26(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_28(...) \
	CPP_SEQUENCE_27(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_29(...) \
	CPP_SEQUENCE_28(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_30(...) \
	CPP_SEQUENCE_29(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_31(...) \
	CPP_SEQUENCE_30(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_32(...) \
	CPP_SEQUENCE_31(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_33(...) \
	CPP_SEQUENCE_32(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_34(...) \
	CPP_SEQUENCE_33(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_35(...) \
	CPP_SEQUENCE_34(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_36(...) \
	CPP_SEQUENCE_35(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_37(...) \
	CPP_SEQUENCE_36(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_38(...) \
	CPP_SEQUENCE_37(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_39(...) \
	CPP_SEQUENCE_38(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_40(...) \
	CPP_SEQUENCE_39(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_41(...) \
	CPP_SEQUENCE_40(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_42(...) \
	CPP_SEQUENCE_41(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_43(...) \
	CPP_SEQUENCE_42(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_44(...) \
	CPP_SEQUENCE_43(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_45(...) \
	CPP_SEQUENCE_44(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_46(...) \
	CPP_SEQUENCE_45(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_47(...) \
	CPP_SEQUENCE_46(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_48(...) \
	CPP_SEQUENCE_47(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_49(...) \
	CPP_SEQUENCE_48(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_50(...) \
	CPP_SEQUENCE_49(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_51(...) \
	CPP_SEQUENCE_50(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_52(...) \
	CPP_SEQUENCE_51(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_53(...) \
	CPP_SEQUENCE_52(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_54(...) \
	CPP_SEQUENCE_53(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_55(...) \
	CPP_SEQUENCE_54(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_56(...) \
	CPP_SEQUENCE_55(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_57(...) \
	CPP_SEQUENCE_56(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_58(...) \
	CPP_SEQUENCE_57(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_59(...) \
	CPP_SEQUENCE_58(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_60(...) \
	CPP_SEQUENCE_59(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_61(...) \
	CPP_SEQUENCE_60(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_62(...) \
	CPP_SEQUENCE_61(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_63(...) \
	CPP_SEQUENCE_62(__VA_ARGS__) __VA_ARGS__
#if __cplusplus >= 201103L
#define CPP_SEQUENCE_64(...) \
	CPP_SEQUENCE_63(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_65(...) \
	CPP_SEQUENCE_64(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_66(...) \
	CPP_SEQUENCE_65(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_67(...) \
	CPP_SEQUENCE_66(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_68(...) \
	CPP_SEQUENCE_67(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_69(...) \
	CPP_SEQUENCE_68(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_70(...) \
	CPP_SEQUENCE_69(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_71(...) \
	CPP_SEQUENCE_70(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_72(...) \
	CPP_SEQUENCE_71(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_73(...) \
	CPP_SEQUENCE_72(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_74(...) \
	CPP_SEQUENCE_73(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_75(...) \
	CPP_SEQUENCE_74(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_76(...) \
	CPP_SEQUENCE_75(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_77(...) \
	CPP_SEQUENCE_76(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_78(...) \
	CPP_SEQUENCE_77(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_79(...) \
	CPP_SEQUENCE_78(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_80(...) \
	CPP_SEQUENCE_79(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_81(...) \
	CPP_SEQUENCE_80(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_82(...) \
	CPP_SEQUENCE_81(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_83(...) \
	CPP_SEQUENCE_82(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_84(...) \
	CPP_SEQUENCE_83(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_85(...) \
	CPP_SEQUENCE_84(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_86(...) \
	CPP_SEQUENCE_85(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_87(...) \
	CPP_SEQUENCE_86(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_88(...) \
	CPP_SEQUENCE_87(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_89(...) \
	CPP_SEQUENCE_88(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_90(...) \
	CPP_SEQUENCE_89(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_91(...) \
	CPP_SEQUENCE_90(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_92(...) \
	CPP_SEQUENCE_91(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_93(...) \
	CPP_SEQUENCE_92(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_94(...) \
	CPP_SEQUENCE_93(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_95(...) \
	CPP_SEQUENCE_94(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_96(...) \
	CPP_SEQUENCE_95(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_97(...) \
	CPP_SEQUENCE_96(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_98(...) \
	CPP_SEQUENCE_97(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_99(...) \
	CPP_SEQUENCE_98(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_100(...) \
	CPP_SEQUENCE_99(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_101(...) \
	CPP_SEQUENCE_100(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_102(...) \
	CPP_SEQUENCE_101(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_103(...) \
	CPP_SEQUENCE_102(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_104(...) \
	CPP_SEQUENCE_103(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_105(...) \
	CPP_SEQUENCE_104(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_106(...) \
	CPP_SEQUENCE_105(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_107(...) \
	CPP_SEQUENCE_106(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_108(...) \
	CPP_SEQUENCE_107(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_109(...) \
	CPP_SEQUENCE_108(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_110(...) \
	CPP_SEQUENCE_109(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_111(...) \
	CPP_SEQUENCE_110(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_112(...) \
	CPP_SEQUENCE_111(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_113(...) \
	CPP_SEQUENCE_112(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_114(...) \
	CPP_SEQUENCE_113(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_115(...) \
	CPP_SEQUENCE_114(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_116(...) \
	CPP_SEQUENCE_115(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_117(...) \
	CPP_SEQUENCE_116(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_118(...) \
	CPP_SEQUENCE_117(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_119(...) \
	CPP_SEQUENCE_118(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_120(...) \
	CPP_SEQUENCE_119(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_121(...) \
	CPP_SEQUENCE_120(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_122(...) \
	CPP_SEQUENCE_121(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_123(...) \
	CPP_SEQUENCE_122(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_124(...) \
	CPP_SEQUENCE_123(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_125(...) \
	CPP_SEQUENCE_124(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_126(...) \
	CPP_SEQUENCE_125(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_127(...) \
	CPP_SEQUENCE_126(__VA_ARGS__) __VA_ARGS__
#define CPP_SEQUENCE_128(...) \
	CPP_SEQUENCE_127(__VA_ARGS__) __VA_ARGS__
#endif // __cplusplus >= 201103L

/**
 * Evaluates to 1 if an empty argument list is provided, and 0 if not.
 *
 * @see CPP_SIZE()
 */
#define CPP_EMPTY(...) \
	CPP_AND(CPP_NOT(CPP_HAS_COMMA(__VA_ARGS__)), \
	CPP_AND(CPP_NOT(CPP_HAS_COMMA(CPP_EMPTY_ __VA_ARGS__)), \
	CPP_AND(CPP_NOT(CPP_HAS_COMMA(__VA_ARGS__ ())), \
	CPP_HAS_COMMA(CPP_EMPTY_ __VA_ARGS__ ()))))
#define CPP_EMPTY_(...) \
	,

/**
 * Evaluates to the number of arguments. The implementation counts the number of
 * commas in the argument list and therefore makes no distinction between a
 * single argument and an empty argument list.
 *
 * @see CPP_EMPTY()
 */
#if __cplusplus >= 201103L
#define CPP_SIZE(...) \
	CPP_AT_127(__VA_ARGS__, \
			127, 126, 125, 124, 123, 122, 121, 120, \
			119, 118, 117, 116, 115, 114, 113, 112, \
			111, 110, 109, 108, 107, 106, 105, 104, \
			103, 102, 101, 100, 99, 98, 97, 96, \
			95, 94, 93, 92, 91, 90, 89, 88, \
			87, 86, 85, 84, 83, 82, 81, 80, \
			79, 78, 77, 76, 75, 74, 73, 72, \
			71, 70, 69, 68, 67, 66, 65, 64, \
			63, 62, 61, 60, 59, 58, 57, 56, \
			55, 54, 53, 52, 51, 50, 49, 48, \
			47, 46, 45, 44, 43, 42, 41, 40, \
			39, 38, 37, 36, 35, 34, 33, 32, \
			31, 30, 29, 28, 27, 26, 25, 24, \
			23, 22, 21, 20, 19, 18, 17, 16, \
			15, 14, 13, 12, 11, 10, 9, 8, \
			7, 6, 5, 4, 3, 2, 1)
#else
#define CPP_SIZE(...) \
	CPP_AT_63(__VA_ARGS__, \
			63, 62, 61, 60, 59, 58, 57, 56, \
			55, 54, 53, 52, 51, 50, 49, 48, \
			47, 46, 45, 44, 43, 42, 41, 40, \
			39, 38, 37, 36, 35, 34, 33, 32, \
			31, 30, 29, 28, 27, 26, 25, 24, \
			23, 22, 21, 20, 19, 18, 17, 16, \
			15, 14, 13, 12, 11, 10, 9, 8, \
			7, 6, 5, 4, 3, 2, 1)
#endif // __cplusplus >= 201103L

/**
 * The maximum size of a preprocessor sequence. This is half the maximum number
 * of parameters in one macro definition (guaranteed to be at least 127 for C
 * and 256 for C++).
 */
#if __cplusplus >= 201103L
#define CPP_MAX_SIZE	128
#else
#define CPP_MAX_SIZE	63
#endif

/// Evaluates to the n-th argument, starting from 0.
#define CPP_AT(n, ...) \
	CPP_EVAL(CPP_HEAD CPP_SEQUENCE(n, CPP_AT_)(__VA_ARGS__))
#define CPP_AT_(...) \
	(CPP_TAIL(__VA_ARGS__))

/**
 * Evaluates to 1 if <b>a</b> and <b>b</b> are equal non-negative integers, and
 * 0 if not.
 *
 * @see CPP_NEQ()
 */
#define CPP_EQ(a, b) \
	CPP_EQ_(a, b)
#define CPP_EQ_(a, b) \
	CPP_HEAD(CPP_TAIL(CPP_EQ_##a##_##b 1, 0))
#define CPP_EQ__ \
	,
#define CPP_EQ_0_0 \
	,
#define CPP_EQ_1_1 \
	,
#define CPP_EQ_2_2 \
	,
#define CPP_EQ_3_3 \
	,
#define CPP_EQ_4_4 \
	,
#define CPP_EQ_5_5 \
	,
#define CPP_EQ_6_6 \
	,
#define CPP_EQ_7_7 \
	,
#define CPP_EQ_8_8 \
	,
#define CPP_EQ_9_9 \
	,
#define CPP_EQ_10_10 \
	,
#define CPP_EQ_11_11 \
	,
#define CPP_EQ_12_12 \
	,
#define CPP_EQ_13_13 \
	,
#define CPP_EQ_14_14 \
	,
#define CPP_EQ_15_15 \
	,
#define CPP_EQ_16_16 \
	,
#define CPP_EQ_17_17 \
	,
#define CPP_EQ_18_18 \
	,
#define CPP_EQ_19_19 \
	,
#define CPP_EQ_20_20 \
	,
#define CPP_EQ_21_21 \
	,
#define CPP_EQ_22_22 \
	,
#define CPP_EQ_23_23 \
	,
#define CPP_EQ_24_24 \
	,
#define CPP_EQ_25_25 \
	,
#define CPP_EQ_26_26 \
	,
#define CPP_EQ_27_27 \
	,
#define CPP_EQ_28_28 \
	,
#define CPP_EQ_29_29 \
	,
#define CPP_EQ_30_30 \
	,
#define CPP_EQ_31_31 \
	,
#define CPP_EQ_32_32 \
	,
#define CPP_EQ_33_33 \
	,
#define CPP_EQ_34_34 \
	,
#define CPP_EQ_35_35 \
	,
#define CPP_EQ_36_36 \
	,
#define CPP_EQ_37_37 \
	,
#define CPP_EQ_38_38 \
	,
#define CPP_EQ_39_39 \
	,
#define CPP_EQ_40_40 \
	,
#define CPP_EQ_41_41 \
	,
#define CPP_EQ_42_42 \
	,
#define CPP_EQ_43_43 \
	,
#define CPP_EQ_44_44 \
	,
#define CPP_EQ_45_45 \
	,
#define CPP_EQ_46_46 \
	,
#define CPP_EQ_47_47 \
	,
#define CPP_EQ_48_48 \
	,
#define CPP_EQ_49_49 \
	,
#define CPP_EQ_50_50 \
	,
#define CPP_EQ_51_51 \
	,
#define CPP_EQ_52_52 \
	,
#define CPP_EQ_53_53 \
	,
#define CPP_EQ_54_54 \
	,
#define CPP_EQ_55_55 \
	,
#define CPP_EQ_56_56 \
	,
#define CPP_EQ_57_57 \
	,
#define CPP_EQ_58_58 \
	,
#define CPP_EQ_59_59 \
	,
#define CPP_EQ_60_60 \
	,
#define CPP_EQ_61_61 \
	,
#define CPP_EQ_62_62 \
	,
#define CPP_EQ_63_63 \
	,
#if __cplusplus >= 201103L
#define CPP_EQ_64_64 \
	,
#define CPP_EQ_65_65 \
	,
#define CPP_EQ_66_66 \
	,
#define CPP_EQ_67_67 \
	,
#define CPP_EQ_68_68 \
	,
#define CPP_EQ_69_69 \
	,
#define CPP_EQ_70_70 \
	,
#define CPP_EQ_71_71 \
	,
#define CPP_EQ_72_72 \
	,
#define CPP_EQ_73_73 \
	,
#define CPP_EQ_74_74 \
	,
#define CPP_EQ_75_75 \
	,
#define CPP_EQ_76_76 \
	,
#define CPP_EQ_77_77 \
	,
#define CPP_EQ_78_78 \
	,
#define CPP_EQ_79_79 \
	,
#define CPP_EQ_80_80 \
	,
#define CPP_EQ_81_81 \
	,
#define CPP_EQ_82_82 \
	,
#define CPP_EQ_83_83 \
	,
#define CPP_EQ_84_84 \
	,
#define CPP_EQ_85_85 \
	,
#define CPP_EQ_86_86 \
	,
#define CPP_EQ_87_87 \
	,
#define CPP_EQ_88_88 \
	,
#define CPP_EQ_89_89 \
	,
#define CPP_EQ_90_90 \
	,
#define CPP_EQ_91_91 \
	,
#define CPP_EQ_92_92 \
	,
#define CPP_EQ_93_93 \
	,
#define CPP_EQ_94_94 \
	,
#define CPP_EQ_95_95 \
	,
#define CPP_EQ_96_96 \
	,
#define CPP_EQ_97_97 \
	,
#define CPP_EQ_98_98 \
	,
#define CPP_EQ_99_99 \
	,
#define CPP_EQ_100_100 \
	,
#define CPP_EQ_101_101 \
	,
#define CPP_EQ_102_102 \
	,
#define CPP_EQ_103_103 \
	,
#define CPP_EQ_104_104 \
	,
#define CPP_EQ_105_105 \
	,
#define CPP_EQ_106_106 \
	,
#define CPP_EQ_107_107 \
	,
#define CPP_EQ_108_108 \
	,
#define CPP_EQ_109_109 \
	,
#define CPP_EQ_110_110 \
	,
#define CPP_EQ_111_111 \
	,
#define CPP_EQ_112_112 \
	,
#define CPP_EQ_113_113 \
	,
#define CPP_EQ_114_114 \
	,
#define CPP_EQ_115_115 \
	,
#define CPP_EQ_116_116 \
	,
#define CPP_EQ_117_117 \
	,
#define CPP_EQ_118_118 \
	,
#define CPP_EQ_119_119 \
	,
#define CPP_EQ_120_120 \
	,
#define CPP_EQ_121_121 \
	,
#define CPP_EQ_122_122 \
	,
#define CPP_EQ_123_123 \
	,
#define CPP_EQ_124_124 \
	,
#define CPP_EQ_125_125 \
	,
#define CPP_EQ_126_126 \
	,
#define CPP_EQ_127_127 \
	,
#define CPP_EQ_128_128 \
	,
#endif // __cplusplus >= 201103L

/**
 * Evaluates to 0 if <b>a</b> and <b>b</b> are equal non-negative integers, and
 * 1 if not.
 *
 * @see CPP_EQ()
 */
#define CPP_NEQ(a, b) \
	CPP_NOT(CPP_EQ(a, b))

/// Evaluates to 1 if `a > b`, and 0 if not.
#define CPP_GT(a, b) \
	CPP_LT(b, a)

/// Evaluates to 1 if `a >= b`, and 0 if not.
#define CPP_GE(a, b) \
	CPP_NOT(CPP_LT(a, b))

/// Evaluates to 1 if `a <= b`, and 0 if not.
#define CPP_LE(a, b) \
	CPP_NOT(CPP_GT(a, b))

/// Evaluates to 1 if `a < b`, and 0 if not.
#define CPP_LT(a, b) \
	CPP_AT(a, CPP_TAIL(CPP_SEQUENCE(b,, 1) CPP_SEQUENCE(a,, 0), 0))

/// Evaluates to 1 if <b>x</b> evaluates to 0, and 0 if not.
#define CPP_NOT(x) \
	CPP_NOT_(x)
#define CPP_NOT_(x) \
	CPP_HEAD(CPP_TAIL(CPP_NOT_##x 1, 0))
#define CPP_NOT_0 \
	,

/// Evaluates to 1 if both <b>a</b> and <b>b</b> evaluate to 1, and 0 if not.
#define CPP_AND(a, b) \
	CPP_IF_ELSE(a)(CPP_IF_ELSE(b)(1)(0))(0)

/// Evaluates to 1 if either <b>a</b> or <b>b</b> evaluates to 1, and 0 if not.
#define CPP_OR(a, b) \
	CPP_IF_ELSE(a)(1)(CPP_IF_ELSE(b)(1)(0))

/**
 * Expands the next expression in parentheses if and only if <b>c</b> evaluates
 * to <b>1</b>. Use as `CPP_IF(cond)(expr-if-true)`.
 */
#define CPP_IF(c) \
	CPP_IF_(CPP_NOT(CPP_NOT(c)))
#define CPP_IF_(c) \
	CPP_IF__(c)
#define CPP_IF__(c) \
	CPP_IF_##c
#define CPP_IF_0(...)
#define CPP_IF_1(...) \
	__VA_ARGS__

/**
 * Expands the first expression in parentheses if <b>c</b> evaluates to
 * <b>1</b>, and the second if not. Use as
 * `CPP_IF_ELSE(cond)(expr-if-true)(expr-if-false)`.
 */
#define CPP_IF_ELSE(c) \
	CPP_IF_ELSE_(CPP_NOT(CPP_NOT(c)))
#define CPP_IF_ELSE_(c) \
	CPP_IF_ELSE__(c)
#define CPP_IF_ELSE__(c) \
	CPP_IF_ELSE_##c
#define CPP_IF_ELSE_0(...) \
	CPP_IF_1
#define CPP_IF_ELSE_1(...) \
	__VA_ARGS__ CPP_IF_0

/**
 * Left folds the argument list by applying <b>f</b> with <b>x</b> as the
 * initial value.
 *
 * @see CPP_FOLDR()
 */
#define CPP_FOLDL(f, x, ...) \
	CPP_EVAL_2(CPP_FOLD_(CPP_FOLDL_, f, x, __VA_ARGS__))
#define CPP_FOLDL_(f, a, b, ...) \
	(f, (f(CPP_EVAL_1 a, b)), __VA_ARGS__)

/**
 * Right folds the argument list by applying <b>f</b> with <b>x</b> as the
 * initial value.
 *
 * @see CPP_FOLDR()
 */
#define CPP_FOLDR(f, x, ...) \
	CPP_EVAL_2(CPP_FOLD_(CPP_FOLDR_, f, x, CPP_REVERSE(__VA_ARGS__)))
#define CPP_FOLDR_(f, a, b, ...) \
	(f, (f(b, CPP_EVAL_1 a)), __VA_ARGS__)

#define CPP_FOLD_(m, f, x, ...) \
	CPP_EVAL_1 CPP_HEAD(CPP_EVAL(CPP_TAIL CPP_FOLD__(m, f, (x), __VA_ARGS__)))
#define CPP_FOLD__(m, f, x, ...) \
	CPP_SEQUENCE(CPP_SIZE(__VA_ARGS__), m)(f, x, __VA_ARGS__,)

/// Applies <b>f</b> to every argument.
#define CPP_MAP(f, ...) \
	CPP_EVAL(CPP_MAP_(f, __VA_ARGS__))
#define CPP_MAP_(f, ...) \
	CPP_IF_ELSE(CPP_HAS_COMMA(__VA_ARGS__))( \
		CPP_MAP__(f, __VA_ARGS__) \
	)( \
		f(__VA_ARGS__) \
	)
#define CPP_MAP__(f, x, ...) \
	f(x), CPP_MAP___ CPP_EVAL_0 CPP_EVAL_0 () () () (f, __VA_ARGS__)
#define CPP_MAP___() \
	CPP_MAP_

/// Reverses the argument list.
#define CPP_REVERSE(...) \
	CPP_EVAL(CPP_REVERSE_((), __VA_ARGS__))
#define CPP_REVERSE_(s, ...) \
	CPP_IF_ELSE(CPP_HAS_COMMA(__VA_ARGS__))( \
		CPP_REVERSE__(s, __VA_ARGS__) \
	)( \
		__VA_ARGS__ CPP_EVAL_1 s \
	)
#define CPP_REVERSE__(s, x, ...) \
	CPP_REVERSE___ CPP_EVAL_0 CPP_EVAL_0 () () () ((, x CPP_EVAL_1 s), __VA_ARGS__)
#define CPP_REVERSE___() \
	CPP_REVERSE_

/// Evaluates its arguments as many times as possible.
#if __cplusplus >= 201103L
#define CPP_EVAL \
	CPP_EVAL_256
#define CPP_EVAL_256(...) \
	CPP_EVAL_128(CPP_EVAL_128(__VA_ARGS__))
#define CPP_EVAL_128(...) \
	CPP_EVAL_64(CPP_EVAL_64(__VA_ARGS__))
#else
#define CPP_EVAL \
	CPP_EVAL_64
#endif
#define CPP_EVAL_64(...) \
	CPP_EVAL_32(CPP_EVAL_32(__VA_ARGS__))
#define CPP_EVAL_32(...) \
	CPP_EVAL_16(CPP_EVAL_16(__VA_ARGS__))
#define CPP_EVAL_16(...) \
	CPP_EVAL_8(CPP_EVAL_8(__VA_ARGS__))
#define CPP_EVAL_8(...) \
	CPP_EVAL_4(CPP_EVAL_4(__VA_ARGS__))
#define CPP_EVAL_4(...) \
	CPP_EVAL_2(CPP_EVAL_2(__VA_ARGS__))
#define CPP_EVAL_2(...) \
	CPP_EVAL_1(CPP_EVAL_1(__VA_ARGS__))
/// Evaluates its arguments exactly once.
#define CPP_EVAL_1(...) \
	__VA_ARGS__
/// Does not evaluate its arguments at all.
#define CPP_EVAL_0(...)

/// Evaluates to the first of its arguments.
#define CPP_HEAD(...) \
	CPP_HEAD_(__VA_ARGS__,)
#define CPP_HEAD_(_0, ...) \
	_0

/**
 * Evaluates to all but the first of its arguments. At least two arguments must
 * be provided.
 */
#define CPP_TAIL(...) \
	CPP_TAIL_(__VA_ARGS__)
#define CPP_TAIL_(_0, ...) \
	__VA_ARGS__

#if __cplusplus >= 201103L
#define CPP_AT_127( \
		_0, _1, _2, _3, _4, _5, _6, _7, \
		_8, _9, _10, _11, _12, _13, _14, _15, \
		_16, _17, _18, _19, _20, _21, _22, _23, \
		_24, _25, _26, _27, _28, _29, _30, _31, \
		_32, _33, _34, _35, _36, _37, _38, _39, \
		_40, _41, _42, _43, _44, _45, _46, _47, \
		_48, _49, _50, _51, _52, _53, _54, _55, \
		_56, _57, _58, _59, _60, _61, _62, _63, \
		_64, _65, _66, _67, _68, _69, _70, _71, \
		_72, _73, _74, _75, _76, _77, _78, _79, \
		_80, _81, _82, _83, _84, _85, _86, _87, \
		_88, _89, _90, _91, _92, _93, _94, _95, \
		_96, _97, _98, _99, _100, _101, _102, _103, \
		_104, _105, _106, _107, _108, _109, _110, _111, \
		_112, _113, _114, _115, _116, _117, _118, _119, \
		_120, _121, _122, _123, _124, _125, _126, ...) \
	CPP_AT_127_(__VA_ARGS__,)
#define CPP_AT_127_(_127, ...) \
	_127
#else
#define CPP_AT_63( \
		_0, _1, _2, _3, _4, _5, _6, _7, \
		_8, _9, _10, _11, _12, _13, _14, _15, \
		_16, _17, _18, _19, _20, _21, _22, _23, \
		_24, _25, _26, _27, _28, _29, _30, _31, \
		_32, _33, _34, _35, _36, _37, _38, _39, \
		_40, _41, _42, _43, _44, _45, _46, _47, \
		_48, _49, _50, _51, _52, _53, _54, _55, \
		_56, _57, _58, _59, _60, _61, _62, ...) \
	CPP_AT_63_(__VA_ARGS__,)
#define CPP_AT_63_(_63, ...) \
	_63
#endif

/// Evaluates to 1 if the argument list contains a comma, and 0 if not.
#if __cplusplus >= 201103L
#define CPP_HAS_COMMA(...) \
	CPP_AT_127(__VA_ARGS__, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#else
#define CPP_HAS_COMMA(...) \
	CPP_AT_63(__VA_ARGS__, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#endif // __cplusplus >= 201103L

#endif // __STDC_VERSION__ >= 199901L || __cplusplus >= 201103L

#endif

