// kernel_module.c
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>   // kthread_run, kthread_stop
#include <linux/delay.h>     // msleep
#include <linux/sched.h>

#define AUTHOR  "utn-fra-td3"

// punteros a los hilos
static struct task_struct *thread_hola;
static struct task_struct *thread_chau;

// ---------- Cuerpos de los hilos ----------

static int hilo_hola_fn(void *data)
{
    while (!kthread_should_stop()) {
        printk(KERN_INFO "Hola desde el kernel!\n");
        if (msleep_interruptible(500))
            break; // si nos despiertan por stop, salimos
    }
    return 0;
}

static int hilo_chau_fn(void *data)
{
    while (!kthread_should_stop()) {
        printk(KERN_INFO "Chau desde el kernel!\n");
        if (msleep_interruptible(500))
            break;
    }
    return 0;
}

// ---------- Ciclo de vida del módulo ----------

static int __init kernel_module_init(void)
{
    int err = 0;

    // crear hilos
    thread_hola = kthread_run(hilo_hola_fn, NULL, "td3_hilo_hola");
    if (IS_ERR(thread_hola)) {
        err = PTR_ERR(thread_hola);
        pr_err("No pude crear td3_hilo_hola: %d\n", err);
        thread_hola = NULL;
        return err;
    }

    thread_chau = kthread_run(hilo_chau_fn, NULL, "td3_hilo_chau");
    if (IS_ERR(thread_chau)) {
        err = PTR_ERR(thread_chau);
        pr_err("No pude crear td3_hilo_chau: %d\n", err);
        thread_chau = NULL;
        // si el segundo falla, detenemos el primero
        kthread_stop(thread_hola);
        thread_hola = NULL;
        return err;
    }

    pr_info("%s: modulo cargado, hilos iniciados\n", AUTHOR);
    return 0;
}

static void __exit kernel_module_exit(void)
{
    if (thread_hola) {
        kthread_stop(thread_hola);
        thread_hola = NULL;
    }
    if (thread_chau) {
        kthread_stop(thread_chau);
        thread_chau = NULL;
    }
    pr_info("%s: modulo descargado, hilos detenidos\n", AUTHOR);
}

module_init(kernel_module_init);
module_exit(kernel_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION("TP: dos hilos periodicos que loguean cada 500 ms");

