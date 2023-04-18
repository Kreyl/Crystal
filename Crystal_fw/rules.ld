/* Stack rules inclusion.*/
INCLUDE ../rules_stacks.ld

/* Code rules inclusion.*/
INCLUDE ../rules_code.ld

/* Data rules inclusion.*/
INCLUDE ../rules_data.ld

/* Memory rules inclusion.*/
INCLUDE ../rules_memory.ld

 /**************************************
 *  Here is place to read and write to *
 *  3C00 is 15th page                  *
 ***************************************/
SECTIONS {
    MyFlash1 0x08003C00 :
    {
        *(MyFlash)
    } > flash0
}