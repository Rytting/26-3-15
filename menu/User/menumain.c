#include "stm32f10x.h"
#include "Delay.h"
#include "Key.h"
#include "TFT_SPI.h"
#include "Question1.h"
static uint8_t runPageJustEntered = 0;
/*==========================
  1. 枚举定义
==========================*/
typedef enum
{
    MENU_MAIN = 0,
    MENU_LIST,
    MENU_DETAIL,
    MENU_RUN
} MenuState;

typedef enum
{
    TYPE_BASIC = 0,
    TYPE_ADVANCED = 1
} QuestionType;

typedef enum
{
    QSTAT_NOT_START = 0,
    QSTAT_RUNNING,
    QSTAT_PAUSED,
    QSTAT_DONE
} QuestionStatus;

typedef struct
{
    char *name;
    char *desc1;
    char *desc2;
    QuestionType type;
    QuestionStatus status;
} QuestionItem;

/*==========================
  2. 全局变量
==========================*/
static MenuState currentMenu = MENU_MAIN;

static uint8_t mainIndex = 0;       // 主菜单索引
static uint8_t listIndex = 0;       // 题目列表索引
static uint8_t detailSel = 0;       // 详情页底部选择 0/1/2
static QuestionType currentType = TYPE_BASIC;
static uint8_t currentQuestion = 0; // 当前选中题号（0~7）

/*==========================
  3. 题目表
==========================*/
static QuestionItem questions[8] =
{
    {"Q1", "RESET TO ORIGIN", "ERR <= 2CM", TYPE_BASIC,    QSTAT_NOT_START},
    {"Q2", "MOVE SQUARE",     "30S CLOCKWISE", TYPE_BASIC, QSTAT_NOT_START},
    {"Q3", "A4 TAPE PATH",    "30S CLOCKWISE", TYPE_BASIC, QSTAT_NOT_START},
    {"Q4", "ROTATED A4",      "30S CLOCKWISE", TYPE_BASIC, QSTAT_NOT_START},

    {"Q1", "TRACK IN 2S",     "GREEN -> RED", TYPE_ADVANCED, QSTAT_NOT_START},
    {"Q2", "TRACK MOVING",    "WITH PAUSE",     TYPE_ADVANCED, QSTAT_NOT_START},
    {"Q3", "ADV RESERVED",    "TODO",           TYPE_ADVANCED, QSTAT_NOT_START},
    {"Q4", "ADV RESERVED",    "TODO",           TYPE_ADVANCED, QSTAT_NOT_START}
};

static const char *mainMenu[2] =
{
    "BASIC",
    "ADVANCED"
};

/*==========================
  4. 工具函数
==========================*/
static uint8_t GetBaseIndexByType(QuestionType type)
{
    return (type == TYPE_BASIC) ? 0 : 4;
}

static char* GetStatusText(QuestionStatus st)
{
    switch(st)
    {
        case QSTAT_NOT_START: return "[ ]";
        case QSTAT_RUNNING:   return "[R]";
        case QSTAT_PAUSED:    return "[P]";
        case QSTAT_DONE:      return "[D]";
        default:              return "[?]";
    }
}

static char* GetStatusName(QuestionStatus st)
{
    switch(st)
    {
        case QSTAT_NOT_START: return "NOT START";
        case QSTAT_RUNNING:   return "RUNNING";
        case QSTAT_PAUSED:    return "PAUSED";
        case QSTAT_DONE:      return "DONE";
        default:              return "UNKNOWN";
    }
}

/*==========================
  5. 题目动作封装
  注意：把“初始化 / 开始 / 停止 / 重置”彻底分开
==========================*/
static void EnterQuestionRun_FirstStart(uint8_t qid)
{
    currentQuestion = qid;
    currentMenu = MENU_RUN;
    runPageJustEntered = 1;
}
//static void EnterQuestionRun_FirstStart(uint8_t qid)
//{
//    currentQuestion = qid;
//    currentMenu = MENU_RUN;

//    TFT_Clear(WHITE);
//    TFT_ShowString(10, 20, "ENTER RUN PAGE", RED);

//    if(qid == 0)
//    {
//        TFT_ShowString(10, 60, "Q1 START", BLUE);
//        /* 先不要调 Question1_Init();
//           先不要调 Question1_SetRunState(Q1_RUNNING); */
//    }
//}

static void ResumeQuestionRun(uint8_t qid)
{
    currentQuestion = qid;
    currentMenu = MENU_RUN;
    runPageJustEntered = 2;
}

static void RestartQuestionRun(uint8_t qid)
{
    currentQuestion = qid;
    currentMenu = MENU_RUN;

    if(qid == 0)
    {
        Question1_Reset();                    // 重新开始才重置
        Question1_SetRunState(Q1_RUNNING);
    }
}

static void StopQuestion(uint8_t qid)
{
    if(qid == 0)
    {
        Question1_SetRunState(Q1_STOPPED);
    }
}

static void ResetQuestion(uint8_t qid)
{
    if(qid == 0)
    {
        Question1_Reset();
    }
}

/*==========================
  6. 显示函数声明
==========================*/
void Show_MainMenu(void);
void Show_ListMenu(void);
void Show_DetailPage(void);
void Show_DetailBottomBar(void);
void Show_RunHintBar(void);

/*==========================
  7. 显示函数实现
==========================*/
void Show_MainMenu(void)
{
    uint8_t i;

    TFT_Clear(BLACK);
    TFT_ShowString(100, 10, "MENU", RED);

    for(i = 0; i < 2; i++)
    {
        if(i == mainIndex)
        {
            TFT_ShowString(70, 50 + i * 35, ">", YELLOW);
            TFT_ShowString(90, 50 + i * 35, (char*)mainMenu[i], YELLOW);
        }
        else
        {
            TFT_ShowString(90, 50 + i * 35, (char*)mainMenu[i], WHITE);
        }
    }

    TFT_ShowString(20, 170, "UP/DN:SELECT", CYAN);
    TFT_ShowString(20, 200, "OK:ENTER", CYAN);
}

void Show_ListMenu(void)
{
    uint8_t i;
    uint8_t base = GetBaseIndexByType(currentType);

    TFT_Clear(BLACK);

    if(currentType == TYPE_BASIC)
        TFT_ShowString(90, 10, "BASIC", RED);
    else
        TFT_ShowString(65, 10, "ADVANCED", RED);

    for(i = 0; i < 4; i++)
    {
        uint8_t qid = base + i;
        uint16_t y = 45 + i * 35;

        if(i == listIndex)
        {
            TFT_ShowString(10, y, ">", YELLOW);
            TFT_ShowString(30, y, questions[qid].name, YELLOW);
            TFT_ShowString(120, y, GetStatusText(questions[qid].status), YELLOW);
        }
        else
        {
            TFT_ShowString(30, y, questions[qid].name, WHITE);
            TFT_ShowString(120, y, GetStatusText(questions[qid].status), CYAN);
        }
    }

    if(listIndex == 4)
    {
        TFT_ShowString(10, 185, ">", YELLOW);
        TFT_ShowString(30, 185, "BACK", YELLOW);
    }
    else
    {
        TFT_ShowString(30, 185, "BACK", WHITE);
    }

    TFT_ShowString(15, 220, "[ ]NEW [R]RUN [P]PAUSE [D]DONE", BLUE);
}

void Show_DetailBottomBar(void)
{
    QuestionStatus st = questions[currentQuestion].status;

    TFT_ClearRect(180, 170, 140, 70, BLACK);

    if(st == QSTAT_NOT_START)
    {
        TFT_ShowString(190, 175, detailSel==0 ? ">START"  : " START", YELLOW);
        TFT_ShowString(190, 195, detailSel==1 ? ">DONE"   : " DONE",  YELLOW);
        TFT_ShowString(190, 215, detailSel==2 ? ">BACK"   : " BACK",  YELLOW);
    }
    else if(st == QSTAT_RUNNING)
    {
        TFT_ShowString(190, 175, detailSel==0 ? ">STOP"   : " STOP",  YELLOW);
        TFT_ShowString(190, 195, detailSel==1 ? ">DONE"   : " DONE",  YELLOW);
        TFT_ShowString(190, 215, detailSel==2 ? ">BACK"   : " BACK",  YELLOW);
    }
    else if(st == QSTAT_PAUSED)
    {
        TFT_ShowString(190, 175, detailSel==0 ? ">RESUME" : " RESUME",YELLOW);
        TFT_ShowString(190, 195, detailSel==1 ? ">DONE"   : " DONE",  YELLOW);
        TFT_ShowString(190, 215, detailSel==2 ? ">BACK"   : " BACK",  YELLOW);
    }
    else if(st == QSTAT_DONE)
    {
        TFT_ShowString(190, 175, detailSel==0 ? ">RESTART": " RESTART",YELLOW);
        TFT_ShowString(190, 195, detailSel==1 ? ">UNDO"   : " UNDO",   YELLOW);
        TFT_ShowString(190, 215, detailSel==2 ? ">BACK"   : " BACK",   YELLOW);
    }
}

void Show_DetailPage(void)
{
    QuestionItem *q = &questions[currentQuestion];

    TFT_Clear(BLACK);

    if(q->type == TYPE_BASIC)
        TFT_ShowString(10, 10, "BASIC", RED);
    else
        TFT_ShowString(10, 10, "ADVANCED", RED);

    TFT_ShowString(160, 10, q->name, YELLOW);

    TFT_ShowString(10, 50, "TASK:", CYAN);
    TFT_ShowString(90, 50, q->desc1, WHITE);
    TFT_ShowString(90, 90, q->desc2, WHITE);
    TFT_ShowString(10, 130, "STATUS:", CYAN);
    TFT_ShowString(140,130, GetStatusName(q->status), GREEN);

    TFT_ShowString(15, 185, "UP/DN-SEL  OK-ACT", BLUE);

    Show_DetailBottomBar();
}

void Show_RunHintBar(void)
{
    TFT_ClearRect(0, 220, 320, 20, WHITE);
    TFT_ShowString(5, 225, "UP RUN/STOP  DN BACK  OK DONE", BLUE);
}

/*==========================
  8. 主函数
==========================*/
int main(void)
{
    Key_State key;
    uint8_t base;

    Delay_Init();
    Key_Init();
    TFT_Init();

    Show_MainMenu();

    while(1)
    {
        key = Key_Scan();

        /* 运行态下持续执行题目任务 */
        if(currentMenu == MENU_RUN)
		{
			if(currentQuestion == 0)
			{
				if(runPageJustEntered == 1)
				{
					Question1_Init();
					Question1_SetRunState(Q1_RUNNING);
					runPageJustEntered = 0;
				}
				else if(runPageJustEntered == 2)
				{
					Question1_ShowRunPage(); 
					Question1_SetRunState(Q1_RUNNING);
					runPageJustEntered = 0;
				}

				Question1_Task();
				Show_RunHintBar();
			}
		}
        if(key == KEY_NONE)
        {
            Delay_ms(10);
            continue;
        }

        /*==========================
          主菜单
        ==========================*/
        if(currentMenu == MENU_MAIN)
        {
            if(key == KEY_UP && mainIndex > 0)
            {
                mainIndex--;
                Show_MainMenu();
            }
            else if(key == KEY_DOWN && mainIndex < 1)
            {
                mainIndex++;
                Show_MainMenu();
            }
            else if(key == KEY_OK)
            {
                currentType = (mainIndex == 0) ? TYPE_BASIC : TYPE_ADVANCED;
                listIndex = 0;
                currentMenu = MENU_LIST;
                Show_ListMenu();
            }
        }

        /*==========================
          题目列表
        ==========================*/
        else if(currentMenu == MENU_LIST)
        {
            if(key == KEY_UP && listIndex > 0)
            {
                listIndex--;
                Show_ListMenu();
            }
            else if(key == KEY_DOWN && listIndex < 4)
            {
                listIndex++;
                Show_ListMenu();
            }
            else if(key == KEY_OK)
            {
                if(listIndex == 4)
                {
                    currentMenu = MENU_MAIN;
                    Show_MainMenu();
                }
                else
                {
                    base = GetBaseIndexByType(currentType);
                    currentQuestion = base + listIndex;
                    detailSel = 0;
                    currentMenu = MENU_DETAIL;
                    Show_DetailPage();
                }
            }
        }

        /*==========================
          详情页
        ==========================*/
        else if(currentMenu == MENU_DETAIL)
        {
            if(key == KEY_UP && detailSel > 0)
            {
                detailSel--;
                Show_DetailBottomBar();
            }
            else if(key == KEY_DOWN && detailSel < 2)
            {
                detailSel++;
                Show_DetailBottomBar();
            }
            else if(key == KEY_OK)
            {
                QuestionStatus *st = &questions[currentQuestion].status;

                if(detailSel == 0)
                {
                    if(*st == QSTAT_NOT_START)
                    {
                        *st = QSTAT_RUNNING;
                        EnterQuestionRun_FirstStart(currentQuestion);
                    }
                    else if(*st == QSTAT_RUNNING)
                    {
                        *st = QSTAT_PAUSED;
                        StopQuestion(currentQuestion);
                        Show_DetailPage();
                    }
                    else if(*st == QSTAT_PAUSED)
                    {
                        *st = QSTAT_RUNNING;
                        ResumeQuestionRun(currentQuestion);
                    }
                    else if(*st == QSTAT_DONE)
                    {
                        *st = QSTAT_RUNNING;
                        RestartQuestionRun(currentQuestion);
                    }
                }
                else if(detailSel == 1)
                {
                    if(*st == QSTAT_DONE)
                    {
                        *st = QSTAT_NOT_START;
                    }
                    else
                    {
                        *st = QSTAT_DONE;
                        StopQuestion(currentQuestion);
                    }
                    Show_DetailPage();
                }
                else if(detailSel == 2)
                {
                    currentMenu = MENU_LIST;
                    Show_ListMenu();
                }
            }
        }

        /*==========================
          运行页
        ==========================*/
        else if(currentMenu == MENU_RUN)
        {
            if(key == KEY_UP)
            {
                if(questions[currentQuestion].status == QSTAT_RUNNING)
                {
                    questions[currentQuestion].status = QSTAT_PAUSED;
                    StopQuestion(currentQuestion);
                }
                else if(questions[currentQuestion].status == QSTAT_PAUSED)
                {
                    questions[currentQuestion].status = QSTAT_RUNNING;
                    ResumeQuestionRun(currentQuestion);
                }

                if(currentQuestion == 0)
                {
                    Show_RunHintBar();
                }
            }
            else if(key == KEY_DOWN)
            {
                currentMenu = MENU_DETAIL;
                Show_DetailPage();
            }
            else if(key == KEY_OK)
            {
                questions[currentQuestion].status = QSTAT_DONE;
                StopQuestion(currentQuestion);
                currentMenu = MENU_DETAIL;
                Show_DetailPage();
            }
        }

        Delay_ms(120);
    }
}
