#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include <linux/mm_types.h>		/* Include free_color_area struct*/
#include <linux/page_color.h>		/* Several global variables */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zhiyuan Zhang");
MODULE_DESCRIPTION("Check Colored Page info");

#define USER_RESERVE_ORDER 10 //10 //8 //18		/* Reserve 1 GB physical pages */ // It is not used anymore

/* DEBUG INFO */
#define TEST_HANDLER 1
#define KERNEL_INSTALL 1
#define RECYCLE 1

/* Introduce functions that are definied in mm/memory.c */
extern int (*assign_colors)(struct mm_struct * mm);
extern struct page * (*colored_alloc)(struct mm_struct * mm, int zero);
extern int (*colored_free)(struct page * page);
extern void (*print_color_usage)(struct mm_struct * mm);	/* Not implemented yet*/
extern void (*collect_task_color)(struct mm_struct * mm);
/* In process color assignment */
extern int (*assign_inProcess_color)(unsigned long long color_mask);
/* For kernel Space */
extern struct page * (*colored_alloc_kernel)(int color);
extern int (*assign_kernel_color)(void);

/* Function Declaration */
int set_process_color(struct mm_struct * mm);		/* Assign a color to a process */
int cache_coloring_init(void);
int create_colored_pool(unsigned int order);		/* Reserve colored pages */
int destroy_colored_pool(void);				/* Free reserved pages */
int destroy_cache_coloring(void);			/* Free color_area, color_lock */
void check_colored_pool(void);				/* Check colored_pool */
struct page * alloc_one_colored_page(struct mm_struct * mm, int zero);
int free_one_colored_page(struct page * page);		/* Free one colored page */
void return_task_color(struct mm_struct * mm);		/* Get color back when a process is finished*/
int assign_color(unsigned long long color_mask);	/* Assign a color in a process */
int refill_colored_pool(unsigned int order);		/* Refill the Colored Pool*/
int isfilled(void);

/* Function declaration for kernel */
int create_colored_pool_kernel(unsigned int order);
void check_colored_pool_kernel(void);
int destroy_colored_pool_kernel(void);	
int destroy_cache_coloring_kernel(void);
struct page * alloc_one_colored_page_kernel(int color);
int refill_colored_pool_kernel(unsigned int order);
int isfilled_kernel(void);

int assign_color_to_kernel(void); // Decide the color
int modulo_color = 20;
int max_pages = 100;

/* Recycle unused pages */
int recycle_pages(void);
int destroy_unused_colored_page(void);

/* Essential variables Declaration */
struct free_color_area *color_area;
spinlock_t * color_lock;
static unsigned long long COLOR_COUNT;			/* Indicate the available color */

struct free_color_area *color_area_kernel;
spinlock_t * color_lock_kernel;

/* Avoid Kernel Memory Leak, take back unused page */
// Or just keep them in the linked-list, either way works
// Do it in an elegant way, free them after each init and refill
struct free_color_area *unused_colored_page;
spinlock_t * color_lock_unused;

/* Helper functions */
/* There is an offset of 1 between mm->color and real assigned color */
#define GET_COLOR(mm_color) (mm_color-1);

#define SLICE_MASK_0 0x1b5f575440UL
#define SLICE_MASK_1 0x2eb5faa880UL
#define SLICE_MASK_2 0x3cccc93100UL

int llc_parity(uint64_t v) { 
  v ^= v >> 1;
  v ^= v >> 2;
  v = (v & 0x1111111111111111UL) * 0x1111111111111111UL;
  return (v >> 60) & 1; 
}

int addr2slice_linear(uint64_t addr, int slices) {
  int bit0 = llc_parity(addr & SLICE_MASK_0);
  int bit1 = llc_parity(addr & SLICE_MASK_1);
  int bit2 = llc_parity(addr & SLICE_MASK_2);
  return ((bit2 << 2) | (bit1 << 1) | bit0) & (slices - 1);
}


unsigned int get_page_color(struct page * page)
{
        unsigned long pfn;
        int slice;
        pfn = page_to_pfn(page);
        slice = addr2slice_linear((uint64_t)(pfn << 12), 8);
	return (pfn & 0xf) + slice * 16; // 1024 cache sets...


        //slice = addr2slice_linear((uint64_t)(pfn << 12), 4);
	//return (pfn & 0x1f) + slice * 32; // 1024 cache sets...
}

static int __init alloc_init(void)
{
#if TEST_HANDLER
	if (NR_COLORS == 32)
		COLOR_COUNT = 0xFFFFFFFF;
	else 
		COLOR_COUNT = 0;

	if ((!cache_coloring_init()) == 1) 
		printk(KERN_INFO "GOOD INIT\n");
	else
		return 1;

	if ((!create_colored_pool(USER_RESERVE_ORDER)) == 1)
		printk(KERN_INFO "GOOD CREATE\n");
	else 
		return 1;
	check_colored_pool();
#if KERNEL_INSTALL
	if ((!create_colored_pool_kernel(USER_RESERVE_ORDER)) == 1)
		printk(KERN_INFO "GOOD CREATE Kernel\n");
	else 
		return 1;
	check_colored_pool_kernel();
#endif
	assign_colors = set_process_color;		/* Hook exec.c*/
	colored_alloc = alloc_one_colored_page;		/* Hook alloc_zeroed_user_highpage_movable */
	colored_free  = free_one_colored_page;		/* Hook free_unref_page */
	collect_task_color = return_task_color;		/* Hook exit_mm */
	assign_inProcess_color = assign_color;		/* Assign color in a process */ 
#if KERNEL_INSTALL
	colored_alloc_kernel = alloc_one_colored_page_kernel;
	assign_kernel_color = assign_color_to_kernel;
#endif
	printk(KERN_INFO "COLOR: Colored Alloc handler has been installed\n");
	return 0;
#endif
	return 0;
}

static void  __exit alloc_cleanup(void)
{
#if TEST_HANDLER
	assign_colors = NULL;
	colored_alloc = NULL;
	colored_free  = NULL; 
	collect_task_color = NULL;
	assign_inProcess_color = NULL; 
	colored_alloc_kernel = NULL;
	assign_kernel_color = NULL;
	printk(KERN_INFO "Unload colored alloc handler, BYE!\n");

	if ((!destroy_colored_pool()) == 1)
		printk(KERN_INFO "GOOD DESTROY\n");
	if ((!destroy_cache_coloring()) == 1)
		printk(KERN_INFO "GOOD DESTROY\n");
#if KERNEL_INSTALL
	if ((!destroy_colored_pool_kernel()) == 1)
		printk(KERN_INFO "Kernel GOOD DESTROY\n");
	if ((!destroy_cache_coloring_kernel()) == 1)
		printk(KERN_INFO "Kernel GOOD DESTROY\n");
#endif

#if RECYCLE
	if ((!destroy_unused_colored_page()) == 1)
		printk(KERN_INFO "Unused Pages GOOD DESTROY\n");
#endif

#endif
}

int set_process_color(struct mm_struct * mm)
{
#if 0
	int i = 0;
	if (mm->color != 0) {
		printk(KERN_INFO "COLOR: Already has an assigned color\n");
		return 1;
	}
	do {
		if (((COLOR_COUNT >> i) & 1) == 1) {
			/* Well, 0 indicates that there is no color assigned */
			mm->color = i+1;
			COLOR_COUNT &= (~(0x1ULL<<i));
		} else
			i += 1;
	} while (mm->color == 0 && i < 32);

	if (mm->color == 0) {
		printk(KERN_INFO "COLOR: We are running out of colors.\n");
		return 1;
	}

	printk("COLOR: Assigned color %lld\n", mm->color);
#endif
	return 0;
}

int cache_coloring_init(void)
{
	int i;

	color_area = kmalloc(sizeof(struct free_color_area)*NR_COLORS,
	GFP_KERNEL);
	if (!color_area) {
		printk(KERN_ERR "COLOR: Cannot init color_area\n");
		return 1;
	}
	color_lock = kmalloc(sizeof(spinlock_t)*NR_COLORS, GFP_KERNEL);
	if (!color_lock) {
		printk(KERN_ERR "COLOR: Cannot init color_lock\n");
		return 1;
	}
	
	for (i = 0; i < NR_COLORS; i++) {
		INIT_LIST_HEAD(&color_area[i].free_list);
		spin_lock_init(&color_lock[i]);
		color_area[i].free_count = 0;
		color_area[i].total_count = 0;
	}

#if KERNEL_INSTALL
	// Initialise the pool for Kernel
	color_area_kernel = kmalloc(sizeof(struct free_color_area)*NR_COLORS,
	GFP_KERNEL);
	if (!color_area_kernel) {
		printk(KERN_ERR "COLOR: Cannot init color_area for kernel\n");
		return 1;
	}
	color_lock_kernel = kmalloc(sizeof(spinlock_t)*NR_COLORS, GFP_KERNEL);
	if (!color_lock_kernel) {
		printk(KERN_ERR "COLOR: Cannot init color_lock for kernel\n");
		return 1;
	}

	for (i = 0; i < NR_COLORS; i++) {
		INIT_LIST_HEAD(&color_area_kernel[i].free_list);
		spin_lock_init(&color_lock_kernel[i]);
		color_area_kernel[i].free_count = 0;
		color_area_kernel[i].total_count = 0;
	}
#endif


#if RECYCLE
	// Initialise the pool for unused page
	unused_colored_page =  kmalloc(sizeof(struct free_color_area)*NR_COLORS,
	GFP_KERNEL);
	if (!unused_colored_page) {
		printk(KERN_ERR "COLOR: Cannot init color_area for unused pages\n");
		return 1;
	}
	color_lock_unused = kmalloc(sizeof(spinlock_t)*NR_COLORS, GFP_KERNEL);
	if (!color_lock_unused) {
		printk(KERN_ERR "COLOR: Cannot init color_lock for unused pages\n");
		return 1;
	}

	for (i = 0; i < NR_COLORS; i++) {
		INIT_LIST_HEAD(&unused_colored_page[i].free_list);
		spin_lock_init(&color_lock_unused[i]);
		unused_colored_page[i].free_count = 0;
		unused_colored_page[i].total_count = 0;
	}
#endif

	printk(KERN_INFO "COLOR: Done init.\n");
	return 0;
}

/* Reserve 1 GB physical memory by Default, call it after the initialisation */
int create_colored_pool(unsigned int order)
{
	unsigned int color;
	struct page * page;
	
	while (isfilled() == 0) {
	//for (i = 0; i < (1ULL << order); i++) {
		page = alloc_page(__GFP_HIGHMEM | __GFP_MOVABLE);
		if (!page) {
			printk(KERN_ERR "COLOR: Failed to reserve memory...\n");
			goto exit_fault;
		}
		page->color_flags = 0xdeadbeef;
		color = get_page_color(page);

		/* Collect unused pages, and release them later */
		if(color_area[color].free_count > max_pages) {
			#if RECYCLE
				spin_lock(&color_lock_unused[color]);
				list_add(&page->lru, &unused_colored_page[color].free_list);
				unused_colored_page[color].free_count++;
				unused_colored_page[color].total_count++;
				spin_unlock(&color_lock_unused[color]);
			#endif

			continue;
		}
		
		spin_lock(&color_lock[color]);
		list_add(&page->lru, &color_area[color].free_list);
		color_area[color].free_count++;
		color_area[color].total_count++;
		spin_unlock(&color_lock[color]);
	}

	#if RECYCLE
		recycle_pages();
	#endif

	printk(KERN_INFO "COLOR: Colored Pool is created.\n");
	return 0;
exit_fault:
	printk(KERN_ERR "COLOR: Cannot establish memory pool...\n");
	return 1;
}

int isfilled(void) {
	int i;
	for (i = 0; i < NR_COLORS; i++) {
		if (color_area[i].free_count < max_pages)
			return 0;
	}
	return 1;
}

void check_colored_pool(void)
{
	int i;
	for (i = 0; i < NR_COLORS; i++)
		printk(KERN_INFO "COLOR %d: Free count: %ld\n", i, color_area[i].free_count);
}

int destroy_colored_pool(void)
{
	int i;
	struct page *page, *tmp;
	for (i = 0; i < NR_COLORS; i++) {		// Get color;
		if (color_area[i].free_count == 0)
			continue;

		spin_lock(&color_lock[i]);
		list_for_each_entry_safe(page, tmp,
		&color_area[i].free_list,lru) {
			page->color_flags = 0;		/* IMPORTANT: set flag back */
			list_del(&page->lru);		/* Del page from list */
			__free_pages(page,0);		/* Free page */
		}
		spin_unlock(&color_lock[i]);
	}
	printk(KERN_INFO "Destroyed colored_pool\n");
	return 0;
}

int destroy_cache_coloring(void)
{
	kfree(color_area);
	kfree(color_lock);
	printk(KERN_INFO "Destroyed color_area and color_lock\n");
	return 0;
}

struct page * alloc_one_colored_page(struct mm_struct * mm, int zero)
{
	struct page * page;
	int color;

	if (mm == NULL) 
		return NULL;
	if (mm->color == 0) 
		return NULL;
	
	/* Check color and assign a page */
	color = GET_COLOR(mm->color);
	if (color_area[color].free_count < 1) {
		if (refill_colored_pool(USER_RESERVE_ORDER) == 1) {
			printk(KERN_INFO "COLOR: Assign fail, OOM\n");
			return NULL;
		}
	}
	spin_lock(&color_lock[color]);
	page = list_entry(color_area[color].free_list.next, struct page, lru);
	if (page == NULL) {
		printk(KERN_INFO "COLOR: Got a NULL page.\n");
		return NULL;
	}

	/* Update color list */
	list_del(&page->lru);
	color_area[color].free_count--;
	spin_unlock(&color_lock[color]);	
	
	if (page != NULL)
		printk(KERN_INFO "COLOR: PID is %d: %s,  has color %d, ASSIGN PAGE 0x%lx with color of %d.\n",
		task_pid_nr(current), current->comm, color, page_to_pfn(page), get_page_color(page));
	if (get_page_color(page) != color) {
		printk(KERN_ERR "Assigned color and needed color don't match. Use default allocation...\n");
		return NULL;
	}
	return page;
}

/*
 * mm/page_alloc.c -> __free_one_page  
 * No matter how many pages we are going to free,
 * This function will finally be called. 
 * ***********************************************
 * Some processes may not be colored(start before this module)
 * so, we don't want to return those pages.
 * Luckily, we have page->color_flags, check this before freeing a page.
 */
int free_one_colored_page(struct page * page)
{
#if 1
	page->color_flags = 0;
	return 1;
#else
	unsigned long flags;
	int color;
	if (page->color_flags == 0xdeadbeef) {
		color = get_page_color(page);
		printk(KERN_INFO "COLOR: Free color: %d\n", color);
		spin_lock_irqsave(&color_lock[color],flags);

		page->lru.next = LIST_POISON1;
		page->lru.prev = LIST_POISON2;
		page->private = 0;

		//list_add(&page->lru, &color_area[color].free_list);
		list_add_tail(&page->lru, &color_area[color].free_list);

		color_area[color].free_count++;
		spin_unlock_irqrestore(&color_lock[color],flags);
		return 0; // <----
	} else {
		//printk(KERN_INFO "This page is not colored. Return it to the slab system\n");
		return 1;
	}
#endif
}


/* kernel/exit.c -> exit_mm() will be called to free mm_struct things */
void return_task_color(struct mm_struct * mm)
{
	int color;
	if (mm->color == 0) {
		//printk(KERN_INFO "COLOR: No color need to be taken back.\n");
		return;
	}
	color = GET_COLOR(mm->color);
	COLOR_COUNT |= (0x1ULL << color); 
	printk(KERN_INFO "COLOR: Return color %d\n", color+1);
}

/* It will be called by a syscall(446) */
int assign_color(unsigned long long bit_mask)
{
	unsigned long long color;
	struct mm_struct * my_mm = current->mm;
	my_mm->color = bit_mask; //0xf;
	color = my_mm->color;
	printk(KERN_INFO "Hello, Assign u a color! --> %lld\n", color);
	return 1;
}

/* Call it when the pool is out of color */
int refill_colored_pool(unsigned int order) 
{
	unsigned int color;
	struct page * page;
	
	while (isfilled() == 0) {
		page = alloc_page(__GFP_HIGHMEM | __GFP_MOVABLE);
		if (!page) {
			printk(KERN_ERR "COLOR: Failed to reserve memory...\n");
			return 1;
		}

		page->color_flags = 0xdeadbeef;
		color = get_page_color(page);
		if(color_area[color].free_count > max_pages) {
			#if RECYCLE
				spin_lock(&color_lock_unused[color]);
				list_add(&page->lru, &unused_colored_page[color].free_list);
				unused_colored_page[color].free_count++;
				unused_colored_page[color].total_count++;
				spin_unlock(&color_lock_unused[color]);
			#endif
			continue;
		}

		spin_lock(&color_lock[color]);
		list_add(&page->lru, &color_area[color].free_list);
		color_area[color].free_count++;
		color_area[color].total_count++;
		spin_unlock(&color_lock[color]);
	}

	#if RECYCLE
		recycle_pages();
	#endif

	printk(KERN_INFO "COLOR: The colored pool is refilled.\n");
	check_colored_pool();
	return 0;
}


int create_colored_pool_kernel(unsigned int order)
{
	unsigned int color;
	struct page * page;
	
	while (isfilled_kernel() == 0) {
		page = alloc_page(GFP_KERNEL);
		if (!page) {
			printk(KERN_ERR "Kernel COLOR: Failed to reserve memory...\n");
			goto exit_fault;
		}
		page->color_flags = 0xdeadbeef;
		color = get_page_color(page);
		if(color_area_kernel[color].free_count > max_pages) {
			#if RECYCLE
				spin_lock(&color_lock_unused[color]);
				list_add(&page->lru, &unused_colored_page[color].free_list);
				unused_colored_page[color].free_count++;
				unused_colored_page[color].total_count++;
				spin_unlock(&color_lock_unused[color]);
			#endif
			continue;
		}
			
		
		spin_lock(&color_lock_kernel[color]);
		list_add(&page->lru, &color_area_kernel[color].free_list);
		color_area_kernel[color].free_count++;
		color_area_kernel[color].total_count++;
		spin_unlock(&color_lock_kernel[color]);
	}

	#if RECYCLE
		recycle_pages();
	#endif

	printk(KERN_INFO "Kernel COLOR: Colored Pool is created.\n");
	return 0;
exit_fault:
	printk(KERN_ERR "Kernel COLOR: Cannot establish memory pool...\n");
	return 1;
}

void check_colored_pool_kernel(void)
{
	int i;
	for (i = 0; i < NR_COLORS; i++)
		printk(KERN_INFO "Kernel COLOR %d: Free count: %ld\n", i, color_area_kernel[i].free_count);
}

int isfilled_kernel(void) {
	int i;
	for (i = 0; i < NR_COLORS; i++) {
		if (color_area_kernel[i].free_count < max_pages)
			return 0;
	}
	return 1;
}

int refill_colored_pool_kernel(unsigned int order) 
{
	unsigned int color;
	struct page * page;
	
	while (isfilled_kernel() == 0) {
		page = alloc_page(GFP_KERNEL);
		if (!page) {
			printk(KERN_ERR "Kernel COLOR: Failed to reserve memory...\n");
			return 1;
		}

		page->color_flags = 0xdeadbeef;
		color = get_page_color(page);
		if(color_area_kernel[color].free_count > max_pages) {
			#if RECYCLE
				spin_lock(&color_lock_unused[color]);
				list_add(&page->lru, &unused_colored_page[color].free_list);
				unused_colored_page[color].free_count++;
				unused_colored_page[color].total_count++;
				spin_unlock(&color_lock_unused[color]);
			#endif
			continue;
		}
			
		spin_lock(&color_lock_kernel[color]);
		list_add(&page->lru, &color_area_kernel[color].free_list);
		color_area_kernel[color].free_count++;
		color_area_kernel[color].total_count++;
		spin_unlock(&color_lock_kernel[color]);
	}

	#if RECYCLE
		recycle_pages();
	#endif

	printk(KERN_INFO "Kernel COLOR: The colored pool is refilled.\n");
	check_colored_pool_kernel();
	return 0;
}

struct page * alloc_one_colored_page_kernel(int color)
{
	struct page * page;

	if (color_area_kernel[color].free_count < 1) {
		if (refill_colored_pool_kernel(USER_RESERVE_ORDER) == 1) {
			printk(KERN_INFO "Kernel COLOR: Assign fail, OOM\n");
			return NULL;
		}
	}

	spin_lock(&color_lock_kernel[color]);
	page = list_entry(color_area_kernel[color].free_list.next, struct page, lru);
	if (page == NULL) {
		printk(KERN_INFO "Kernel COLOR: Got a NULL page.\n");
		return NULL;
	}

	/* Update color list */
	list_del(&page->lru);
	color_area_kernel[color].free_count--;
	spin_unlock(&color_lock_kernel[color]);	
	
	if (page != NULL)
		printk(KERN_INFO "COLOR: PID is %d: %s,  has color %d, ASSIGN PAGE 0x%lx with color of %d.\n",
		task_pid_nr(current), current->comm, color, page_to_pfn(page), get_page_color(page));
	if (get_page_color(page) != color) {
		printk(KERN_ERR "Assigned color and needed color don't match. Use default allocation...\n");
		return NULL;
	}
	return page;
}


int destroy_colored_pool_kernel(void)
{
	int i;
	struct page *page, *tmp;
	for (i = 0; i < NR_COLORS; i++) {		// Get color;
		if (color_area_kernel[i].free_count == 0)
			continue;

		spin_lock(&color_lock_kernel[i]);
		list_for_each_entry_safe(page, tmp,
		&color_area_kernel[i].free_list,lru) {
			page->color_flags = 0;		/* IMPORTANT: set flag back */
			list_del(&page->lru);		/* Del page from list */
			__free_pages(page,0);		/* Free page */
		}
		spin_unlock(&color_lock_kernel[i]);
	}
	printk(KERN_INFO "Destroyed colored_pool for kernel\n");
	return 0;
}


int destroy_cache_coloring_kernel(void)
{
	kfree(color_area_kernel);
	kfree(color_lock_kernel);
	printk(KERN_INFO "Destroyed color_area and color_lock for kernel\n");
	return 0;
}


int recycle_pages(void)
{
	int i;
	struct page *page, *tmp;
	for (i = 0; i < NR_COLORS; i++) {		// Get color;
		if (unused_colored_page[i].free_count == 0)
			continue;

		spin_lock(&color_lock_unused[i]);
		list_for_each_entry_safe(page, tmp,
		&unused_colored_page[i].free_list,lru) {
			page->color_flags = 0;		/* IMPORTANT: set flag back */
			list_del(&page->lru);		/* Del page from list */
			__free_pages(page,0);		/* Free page */
		}
		spin_unlock(&color_lock_unused[i]);
	}
	printk(KERN_INFO "Freed unused pages\n");
	return 0;
}

int assign_color_to_kernel(void)
{
	return modulo_color;
}

int destroy_unused_colored_page(void)
{
	kfree(unused_colored_page);
	kfree(color_lock_unused);
	printk(KERN_INFO "Destroyed color_area and color_lock for unused pages\n");
	return 0;
}

module_init(alloc_init);
module_exit(alloc_cleanup);
