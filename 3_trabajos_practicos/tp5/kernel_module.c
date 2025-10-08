// kernel_module.c
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include "gpio_driver.h"

#define AUTHOR "utn-fra-td3"

// ---------------- Parametro de modulo ----------------
static unsigned int gpio = 18;              // por defecto, GPIO18 (BCM)
module_param(gpio, uint, 0644);
MODULE_PARM_DESC(gpio, "Numero de GPIO BCM a usar como salida LED");

// ---------------- Estado global ----------------
static struct task_struct *t_on;
static struct task_struct *t_off;
static void __iomem *gpio_base;

// ---------------- Hilos ----------------
static int hilo_on_fn(void *data)
{
    /* desfase inicial de 250 ms para intercalar con el hilo OFF */
    if (msleep_interruptible(250))
        return 0;

    while (!kthread_should_stop()) {
        gpio_set(gpio);            // LED ON
        if (msleep_interruptible(500))
            break;
    }
    return 0;
}

static int hilo_off_fn(void *data)
{
    while (!kthread_should_stop()) {
        gpio_clr(gpio);            // LED OFF
        if (msleep_interruptible(500))
            break;
    }
    return 0;
}

// ---------------- Ciclo de vida del modulo ----------------
static int __init kernel_module_init(void)
{
    int err;

    pr_info("%s: cargando; GPIO=%u\n", AUTHOR, gpio);

    // 1) Mapear el bloque de GPIO
    gpio_base = gpio_map();                    // devuelve base o NULL
    if (!gpio_base) {
        pr_err("No pude mapear GPIO\n");
        return -ENOMEM;
    }

    // 2) Configurar como salida y apagar inicialmente
    gpio_set_dir_output(gpio);                 // configura GPFSELn como output
    gpio_clr(gpio);                            // LED en OFF de arranque

    // 3) Crear hilos
    t_off = kthread_run(hilo_off_fn, NULL, "td3_led_off");
    if (IS_ERR(t_off)) {
        err = PTR_ERR(t_off);
        pr_err("No pude crear td3_led_off: %d\n", err);
        t_off = NULL;
        gpio_unmap();
        return err;
    }

    t_on = kthread_run(hilo_on_fn, NULL, "td3_led_on");
    if (IS_ERR(t_on)) {
        err = PTR_ERR(t_on);
        pr_err("No pude crear td3_led_on: %d\n", err);
        t_on = NULL;
        kthread_stop(t_off);
        t_off = NULL;
        gpio_unmap();
        return err;
    }

    pr_info("%s: modulo cargado; hilos iniciados\n", AUTHOR);
    return 0;
}

static void __exit kernel_module_exit(void)
{
    if (t_on) {
        kthread_stop(t_on);
        t_on = NULL;
    }
    if (t_off) {
        kthread_stop(t_off);
        t_off = NULL;
    }

    // Apago LED al salir y libero mapeo
    gpio_clr(gpio);
    gpio_unmap();

    pr_info("%s: modulo descargado; hilos detenidos\n", AUTHOR);
}

module_init(kernel_module_init);
module_exit(kernel_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION("TP5 V2: LED ON/OFF con dos hilos cada 500 ms y GPIO parametrizable");
