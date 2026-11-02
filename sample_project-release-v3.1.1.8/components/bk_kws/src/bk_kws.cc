
#include <algorithm>
#include <cstdint>
#include <iterator>

#define TF_LITE_STATIC_MEMORY

#include "bk_kws.h"
#include "os/os.h"
#include "os/mem.h"

namespace Bk_KWS {
constexpr uint32_t TFLM_BUFF_SIZE = 30 * 1024; // TensorFlow Lite 缓冲区大小
constexpr uint32_t KWS_MODEL_BUFF_SIZE = 28 * 1024;  // 模型缓冲区大小

static char TAG[] = "asr"; 

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

KWS_INFO kws_info;
KWS_INFO *p_kws = &kws_info;

#define KWS_MODEL_SWITCH   (0)

#if 0
// india Heytodu WAKE UP
const int KWS_POST_LIST_TAB[] = {12,8};
const int KWS_POST_A_TAB[]={65000*8,65000*8};
#endif

#if 1
// XZ  XA  XL  WAKE UP
const int KWS_POST_LIST_TAB[] = {8,8};
const int KWS_POST_A_TAB[]={65000*5,65000*5};
#endif

#if KWS_MODEL_SWITCH
const int KWS_POST_B_TAB[]={65000*5,65000*3};
#endif

#if 1 // for XZ  XL wake up
constexpr const char *kCategoryLabels[MAX_KWS_CNT] = {
    "unknown",
    "你好小智",
    "再见小智",
};
#endif

#if 0  // for india tudu wake up and cmds
constexpr const char *kCategoryLabels[MAX_KWS_CNT] =
{
    "unknown",
    "heytodu",
    "play",
    "stop",
    "next",
    "incre",
    "decre",
};
#endif

#if 0   // for XA wake up and cmds
constexpr const char *kCategoryLabels[MAX_KWS_CNT] =
{
    "unknown",
    "xiaoai",  //1
    "play_music", //2
    "stop_playing", //3
    "photo_capture", //4
    "receive_call", //5
    "start_video", //6
    "start_camera",  //7
    "incre_volume",  //8
    "decre_volume", //9
    "last_music", //10
    "next_music", //11
    "noise",      //12
    "NOISE",      //13
};
#endif

typedef enum {
    KWS_MODEL_WAKEUP = 0,
    KWS_MODEL_CMDS,
    KWS_MODEL_RSV,
} KWS_MODED_TYPE;

#define KWS_DUMP_DATA 0

#if KWS_DUMP_DATA
static int dump_flag=0;
static int micval=0;
static int dcnt1=0;
static int dcnt2=0;
void data_dump(int16_t *p0, int32_t size)
{
    int32_t len = size>>1;
    int16_t *p1 = p0;
    int32_t k=0;

    dcnt1++;
    micval = 0;
    for (k = 0; k < len; k++) {
        if (p1[k] > 0) {
            micval += p1[k];
        } else {
            micval -= p1[k];
        }
    }
    micval /= len;

    if (micval > 22000) {
        if ((dcnt1-dcnt2) > 50) {
            dcnt2=dcnt1;
            dump_flag = !dump_flag;
        }
    }

    if (dump_flag) {
        bk_uart_write_bytes(1, (const int16_t*)p1, size);
    }
}
#endif

////////////////////////
extern "C" void* tflm_model_get_for_kws_model(void);
extern "C" void* tflm_resolver_get_for_kws_model(void);
extern "C" char* tflm_version_get_for_kws_model(void);

extern "C" void* tflm_model_get_for_kws_model2(void);
extern "C" void* tflm_resolver_get_for_kws_model2(void);
extern "C" char* tflm_version_get_for_kws_model2(void);

extern "C" int tflm_interpreter_init(void** handle, uint8_t* model, void* resolver, uint32_t tensor_arena_size);
extern "C" int tflm_interpreter_init2(void** handle, uint8_t* model, void* resolver, uint8_t* tensor_arena_buff, uint32_t tensor_arena_size);
extern "C" int tflm_interpreter_deinit(void* handle);
extern "C" void* tflm_interpreter_run(void* handle, int8_t* data);
extern "C" void* tflm_interpreter_output(void* handle, uint32_t index, uint32_t* size);
///////////////////////////////////////

#if 0
static void adc_gain_set(int ch, uint32_t gain)
{
#define ADC1_GAIN_ADD    (0x4401014c)
#define ADC2_GAIN_ADD    (0x4401016c)
#define ANA_GAIN_MSK     (0x0f<<15)
   uint32_t ANA_GAIN_REG = gain<<15;
   uint32_t reg1 = (*((volatile UINT32 *)(ADC1_GAIN_ADD)));
   uint32_t reg2 = (*((volatile UINT32 *)(ADC2_GAIN_ADD)));
   uint32_t val1 = reg1 & ANA_GAIN_MSK;
   uint32_t val2 = reg2 & ANA_GAIN_MSK;
   reg1 &= ~(ANA_GAIN_MSK);
   reg1 |= ANA_GAIN_REG;
   reg2 &= ~(ANA_GAIN_MSK);
   reg2 |= ANA_GAIN_REG;

   if (val1 !=ANA_GAIN_REG) {
      (*((volatile UINT32 *)(ADC1_GAIN_ADD))) = reg1;
       LOGI("set adc1 ana gain, reg: %x, gain: %d\n", reg1, gain);
   }

   if (ch>1) {
      if (val2 != ANA_GAIN_REG) {
         (*((volatile UINT32 *)(ADC2_GAIN_ADD))) = reg2;
          LOGI("set adc2 ana gain, reg: %x, gain: %d\n", reg2, gain);
      }
   }
}
#endif

int GenerateFeatures(const int16_t* audio_data,
                              int8_t **out_data) 
{
    static int print_cnt = 0;
    int ret = 0;
    const int16_t *p = audio_data;
  
    ret = mfcc_calue(&p_kws->mfcc_info, p);
    if (ret != 0 && ++print_cnt > 50) {
        print_cnt = 0;
        LOGE("mfcc calc failed, ret: %d\r\n", ret);
    }

    *out_data = &p_kws->mfcc_info.feat_out[0][0];

    return 0;
}

#if KWS_MODEL_SWITCH
extern "C" int tflm_model_switch(int id)
{
  int err = 0;
  if (id != p_kws->model_id)
  {
     int32_t * post_ptr=NULL;
     uint8_t * pm = NULL;
     void    * pr = NULL;
     char    * pv = NULL;
     int32_t   LN = 0;
     switch (id)
     {
        case KWS_MODEL_WAKEUP:
             post_ptr = (int32_t*)KWS_POST_A_TAB;
             pm = (uint8_t*)tflm_model_get_for_kws_model();
             pr = tflm_resolver_get_for_kws_model();
             pv = tflm_version_get_for_kws_model();
             LN = KWS_POST_LIST_TAB[0];
             break;
        case KWS_MODEL_CMDS:
        default:
             post_ptr = (int32_t*)KWS_POST_B_TAB;
             pm = (uint8_t*)tflm_model_get_for_kws_model2();
             pr = tflm_resolver_get_for_kws_model2();
             pv = tflm_version_get_for_kws_model2();
             LN = KWS_POST_LIST_TAB[1];
             break;
     }
     ///////////  backup before memset
     uint32_t  base = p_kws->mfcc_info.base;
     uint8_t * tf_buff_p = p_kws->tf_buff;
     ///////////
     err = tflm_interpreter_deinit(p_kws->TF_Hdl);
     LOGI("tflm [%d] deninit %d\n",p_kws->model_id,err);
     memset(p_kws,0,sizeof(KWS_INFO));
     mfcc_init(&p_kws->mfcc_info);
     p_kws->mfcc_info.FN = 32;
     p_kws->p_thr_tab1 = (int32_t*)post_ptr;
     p_kws->mfcc_info.base = base;
     p_kws->p_mic_data = (int16_t*)p_kws->mfcc_info.fbuff; //reuse
     p_kws->tf_buff = tf_buff_p;
     err = tflm_interpreter_init2(&p_kws->TF_Hdl, pm, pr, p_kws->tf_buff, KWS_MODEL_BUFF_SIZE);
     uint32_t out_N = 0;
     tflm_interpreter_output(p_kws->TF_Hdl, 0, &out_N);
     p_kws->KN = out_N-1;
     p_kws->LN = LN;
     p_kws->model_id = id;
     LOGI("tflm_init %d  KN %d LN %d ver %s\r\n",err,p_kws->KN,p_kws->LN,pv);
  }
  else
  {
     LOGI("same model id\n");
  }
  return err;
}
#endif

void tflite_recgonize(int16_t *audio_data, uint32_t audio_data_size, const char **text, float *score, int16_t *result)
{
  int8_t *outdata = NULL;
  int8_t *feature = NULL;

  GenerateFeatures(audio_data, &feature);

  outdata = (int8_t *)tflm_interpreter_run(p_kws->TF_Hdl, feature);
  
  *result = 0;

  p_kws->frame_cnt++;

#if KWS_DUMP_DATA
    if (dump_flag==0) {
        LOGI("label1: %d, label2: %d, micval: %d\r\n",outdata[1], outdata[2], micval);
    }
#endif

    // Post Process
    int m, n, val, id = p_kws->idx, thr = p_kws->p_thr_tab1[1];
    /**
     * 1. 将输出值从[-128,127]范围转换到[0,255]范围
     * 2. 将记分存入滑动窗口缓冲区
     */
    for (m = 0; m < p_kws->KN; m++) {
        val = (outdata[m+1] + 128);
        val = val * val;
        p_kws->score_list[m][id] = val;
    }

    val = 256 - (outdata[0] + 128);  ///// noise val
    p_kws->score_list[m][id] = val * val;
    val = 0;
    for (n = 0; n < p_kws->LN; n++) {
        val += p_kws->score_list[m][n];
    }

    // 滑动窗口索引循环
    if(++id == p_kws->LN) {
        id = 0;
    }
    p_kws->idx = id;
    id = -1;

    if (val > p_kws->p_thr_tab1[0]) {
      for (m = 0; m < p_kws->KN; m++) {
        val = 0;
        for (n = 0; n < p_kws->LN; n++) {
           val += p_kws->score_list[m][n];
        }

        if (val > thr) {
          id = m;
          thr = val;
          *text = kCategoryLabels[m+1];
        }
      }
    }

    if (id >= 0) {
        // 消抖，防止短时间内重复触发，只有距离上次成功唤醒超过30帧才认为是有效唤醒
        if (((p_kws->frame_cnt - p_kws->start_flag) > 30)) {
            p_kws->start_flag = p_kws->frame_cnt;
            *result = id + 1;
            LOGI("kws detected, label id: %d, label: %s, cnt: %d\r\n", id, *text, ++p_kws->success_cnt);

            #if KWS_MODEL_SWITCH
            if (p_kws->model_id == KWS_MODEL_WAKEUP) {
                tflm_model_switch(KWS_MODEL_CMDS);
            } else {
                tflm_model_switch(KWS_MODEL_WAKEUP);
            }
            #endif
       }
    }
}

void debugLogCallback(const char *s)
{
    if (KWS_DUMP_DATA == 0) {
       LOGE("%s", s);
    }
}

extern "C" bk_err_t bk_kws_init(void)
{
    int ret = 0;
    uint8_t *kws_tflm_buff = NULL;
    uint32_t out_N = 0;

    LOGI("bk_kws_init, version: %s\r\n", bk_kws_ver());

    memset(p_kws, 0, sizeof(KWS_INFO));
    mfcc_init(&p_kws->mfcc_info);

    p_kws->mfcc_info.FN = 32;
    p_kws->p_thr_tab1 = (int32_t*)KWS_POST_A_TAB;
    p_kws->mfcc_info.base = (uint32_t)CONFIG_PSRAM_MEM_SLAB_KWS_ADDR;
    p_kws->p_mic_data = (int16_t*)p_kws->mfcc_info.fbuff; //reuse
    
    kws_tflm_buff = (uint8_t *)psram_malloc(TFLM_BUFF_SIZE);
    if (kws_tflm_buff == NULL) {
        LOGE("psram malloc fail, size: %d\n",TFLM_BUFF_SIZE);
        goto error;
    }
    
    ret = tflm_interpreter_init2(&p_kws->TF_Hdl, (uint8_t*)tflm_model_get_for_kws_model(), tflm_resolver_get_for_kws_model(), kws_tflm_buff, KWS_MODEL_BUFF_SIZE);
    if (ret != 0) {
        LOGE("tflite micro interpreter init fail, ret: %d\n", ret);
        goto error;
    }
    p_kws->tf_buff = (uint8_t*)kws_tflm_buff; // backup

    tflm_interpreter_output(p_kws->TF_Hdl, 0, &out_N);
    p_kws->KN = out_N - 1;
    p_kws->LN = KWS_POST_LIST_TAB[0];
    LOGI("tflm version: %s, KN: %d, LN: %d, mfcc version: %u\n", tflm_version_get_for_kws_model(), p_kws->KN, p_kws->LN, mfcc_ver());

    LOGI("tflite micro interpreter init success\n");

    // adc_gain_set(1,8);

    //RegisterDebugLogCallback(debugLogCallback);

    return BK_OK;

error:
    if (kws_tflm_buff != NULL) {
        psram_free(kws_tflm_buff);
    }

    return BK_FAIL;
}

extern "C" int bk_kws_recog(void *buf, int buf_len, const char **text, float *score)
{
    int16_t result = -1;

    if (buf_len != KWS_FRAME_SIZE) {
        LOGE("ksw frame len mismatch, len: %d, expect: %d\n", buf_len, KWS_FRAME_SIZE);
        return -1;
    }

    memcpy(p_kws->p_mic_data, buf, KWS_FRAME_SIZE);

#if KWS_DUMP_DATA
    data_dump(p_kws->p_mic_data, KWS_FRAME_SIZE);
#endif
    
    tflite_recgonize(p_kws->p_mic_data, KWS_FRAME_SIZE, text, score, &result);
    
    if (result > 0) {
        return 1;
    }

    return 0;
}

 }  // namespace

