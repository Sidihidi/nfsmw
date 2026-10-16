#ifndef RCMP_SOURCE_DECODER_PS2_RCMP_PS2_SONY_CODEC_H
#define RCMP_SOURCE_DECODER_PS2_RCMP_PS2_SONY_CODEC_H

// El analizador de cabeceras MPEG-2 del codec de PS2 de rcmp (decoder/cmn/getbits.cpp y gethdr.cpp): el
// decodificador de referencia del MPEG Software Simulation Group que EA adapto, con sus globales prefijados MPD.
// Tipos y globales de los stabs de la A124; los globales los define rcmp_ps2_sony_codec.cpp.

#define PICTURE_START_CODE 0x100
#define SEQUENCE_HEADER_CODE 0x1B3
#define SEQUENCE_END_CODE 0x1B7
#define GROUP_START_CODE 0x1B8
#define EXTENSION_START_CODE 0x1B5
#define USER_DATA_START_CODE 0x1B2

#define SEQUENCE_EXTENSION_ID 1
#define SEQUENCE_DISPLAY_EXTENSION_ID 2
#define QUANT_MATRIX_EXTENSION_ID 3
#define COPYRIGHT_EXTENSION_ID 4
#define SEQUENCE_SCALABLE_EXTENSION_ID 5
#define PICTURE_DISPLAY_EXTENSION_ID 7
#define PICTURE_CODING_EXTENSION_ID 8
#define PICTURE_SPATIAL_SCALABLE_EXTENSION_ID 9
#define PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID 10

#define I_TYPE 1
#define P_TYPE 2
#define B_TYPE 3

#define FRAME_PICTURE 3

#define SC_NONE 0
#define SC_SPAT 2

#define ZIG_ZAG 0

#define PROFILE_422 (128 + 5)
#define MAIN_LEVEL 8

struct layer_data {
    unsigned char *Rdptr;                        // offset 0x0
    unsigned int Bfr;                            // offset 0x4
    int Incnt;                                   // offset 0x8
    int Bitcnt;                                  // offset 0xC
    int intra_quantizer_matrix[64];              // offset 0x10
    int non_intra_quantizer_matrix[64];          // offset 0x110
    int chroma_intra_quantizer_matrix[64];       // offset 0x210
    int chroma_non_intra_quantizer_matrix[64];   // offset 0x310
    int load_intra_quantizer_matrix;             // offset 0x410
    int load_non_intra_quantizer_matrix;         // offset 0x414
    int load_chroma_intra_quantizer_matrix;      // offset 0x418
    int load_chroma_non_intra_quantizer_matrix;  // offset 0x41C
    int MPEG2_Flag;                              // offset 0x420
    int scalable_mode;                           // offset 0x424
    int q_scale_type;                            // offset 0x428
    int alternate_scan;                          // offset 0x42C
    int pict_scal;                               // offset 0x430
    int priority_breakpoint;                     // offset 0x434
    int quantizer_scale;                         // offset 0x438
    int intra_slice;                             // offset 0x43C
    short block[12][64];                         // offset 0x440
};

extern unsigned char MPDscan[2][64];
extern unsigned char MPDdefault_intra_quantizer_matrix[64];

extern int MPDhorizontal_size;
extern int MPDvertical_size;
extern int MPDprofile;
extern int MPDlevel;
extern float MPDbit_rate;
extern float MPDframe_rate;
extern int MPDaspect_ratio_information;
extern int MPDframe_rate_code;
extern int MPDbit_rate_value;
extern int MPDvbv_buffer_size;
extern int MPDconstrained_parameters_flag;
extern int MPDprofile_and_level_indication;
extern int MPDprogressive_sequence;
extern int MPDchroma_format;
extern int MPDlow_delay;
extern int MPDframe_rate_extension_n;
extern int MPDframe_rate_extension_d;
extern int MPDvideo_format;
extern int MPDcolor_description;
extern int MPDcolor_primaries;
extern int MPDtransfer_characteristics;
extern int MPDmatrix_coefficients;
extern int MPDdisplay_horizontal_size;
extern int MPDdisplay_vertical_size;
extern int MPDtemporal_reference;
extern int MPDpicture_coding_type;
extern int MPDvbv_delay;
extern int MPDfull_pel_forward_vector;
extern int MPDforward_f_code;
extern int MPDfull_pel_backward_vector;
extern int MPDbackward_f_code;
extern int MPDf_code[2][2];
extern int MPDintra_dc_precision;
extern int MPDpicture_structure;
extern int MPDtop_field_first;
extern int MPDframe_pred_frame_dct;
extern int MPDconcealment_motion_vectors;
extern int MPDintra_vlc_format;
extern int MPDrepeat_first_field;
extern int MPDchroma_420_type;
extern int MPDprogressive_frame;
extern int MPDcomposite_display_flag;
extern int MPDv_axis;
extern int MPDfield_sequence;
extern int MPDsub_carrier;
extern int MPDburst_amplitude;
extern int MPDsub_carrier_phase;
extern int MPDframe_center_horizontal_offset[3];
extern int MPDframe_center_vertical_offset[3];
extern int MPDlayer_id;
extern int MPDlower_layer_prediction_horizontal_size;
extern int MPDlower_layer_prediction_vertical_size;
extern int MPDhorizontal_subsampling_factor_m;
extern int MPDhorizontal_subsampling_factor_n;
extern int MPDvertical_subsampling_factor_m;
extern int MPDvertical_subsampling_factor_n;
extern int MPDlower_layer_temporal_reference;
extern int MPDlower_layer_horizontal_offset;
extern int MPDlower_layer_vertical_offset;
extern int MPDspatial_temporal_weight_code_table_index;
extern int MPDlower_layer_progressive_frame;
extern int MPDlower_layer_deinterlaced_field_select;
extern int MPDcopyright_flag;
extern int MPDcopyright_identifier;
extern int MPDoriginal_or_copy;
extern int MPDcopyright_number_1;
extern int MPDcopyright_number_2;
extern int MPDcopyright_number_3;
extern int MPDdrop_flag;
extern int MPDhour;
extern int MPDminute;
extern int MPDsec;
extern int MPDframe;
extern int MPDclosed_gop;
extern int MPDbroken_link;
extern layer_data MPDbase;
extern layer_data *MPDld;
extern int MPDtrue_framenum;

// getbits.cpp
void Initialize_Buffer(unsigned char *bitstream);
int Show_Bits(int N);
int Get_Bits1();
void Flush_Buffer(int N);
int Get_Bits(int N);
void Flush_Buffer32();
int Get_Bits32();

// gethdr.cpp
int Get_Hdr();
void next_start_code();
void marker_bit(char *text);

#endif
