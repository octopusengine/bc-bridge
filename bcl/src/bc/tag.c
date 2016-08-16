#include <bc/tag.h>

void bc_tag_transfer_init(bc_tag_transfer_t *transfer)
{
	memset(transfer, 0, sizeof(*transfer));
}
