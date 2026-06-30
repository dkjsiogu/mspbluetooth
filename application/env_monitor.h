/*
 * env_monitor.h
 * 多点温度/环境监测系统应用层。模块把传感器采集、OLED 显示、蓝牙通信、
 * Flash 历史记录、蜂鸣器报警和编码器阈值调节组织成一个前台调度器。
 */
#ifndef ENV_MONITOR_H
#define ENV_MONITOR_H

/* env_monitor_init: 初始化应用状态并输出启动信息。 */
void env_monitor_init(void);

/* env_monitor_service: 前台循环服务函数，需要在 main 的 while(1) 中持续调用。 */
void env_monitor_service(void);

#endif
