#pragma once

typedef void (*devintr_callback)();
struct devintr_map {
	int irq;
	devintr_callback callback;
};

struct devintr_map* devintr_register_callback(int irq, devintr_callback callback);
