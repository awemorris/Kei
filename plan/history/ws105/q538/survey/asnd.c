#include <stdio.h>
#include <sys/ioctl.h>
#include <sound/asound.h>
int main(void) {
	printf("CARD_INFO=%#lx ELEM_LIST=%#lx ELEM_INFO=%#lx ELEM_READ=%#lx ELEM_WRITE=%#lx SUBSCRIBE_EVENTS=%#lx\n",
		(unsigned long)SNDRV_CTL_IOCTL_CARD_INFO, (unsigned long)SNDRV_CTL_IOCTL_ELEM_LIST, (unsigned long)SNDRV_CTL_IOCTL_ELEM_INFO,
		(unsigned long)SNDRV_CTL_IOCTL_ELEM_READ, (unsigned long)SNDRV_CTL_IOCTL_ELEM_WRITE, (unsigned long)SNDRV_CTL_IOCTL_SUBSCRIBE_EVENTS);
	printf("sizeof id=%zu list=%zu info=%zu value=%zu event=%zu card_info=%zu\n", sizeof(struct snd_ctl_elem_id), sizeof(struct snd_ctl_elem_list),
		sizeof(struct snd_ctl_elem_info), sizeof(struct snd_ctl_elem_value), sizeof(struct snd_ctl_event), sizeof(struct snd_ctl_card_info));
	return 0;
}
