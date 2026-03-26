// #include "superblock.h"
//
// #include <vm/kmalloc.h>
//
// void sb_inc_ref(super_block_t* sb) {
//     __sync_fetch_and_add(&sb->ref_count, 1);
// }
//
// void sb_dec_ref(super_block_t* sb) {
//     if (__sync_fetch_and_sub(&sb->ref_count, 1) == 1) {
//         sb_destroy(sb);
//     }
// }
//
// super_block_t* sb_create(file_system_type_t* fs_type, sb_ops_t* sb_ops) {
//     if (!fs_type || !sb_ops) return NULL;
//
//     super_block_t* sb = kmalloc(sizeof(super_block_t));
//     if (!sb) return NULL;
//
//     sb->fs_type = fs_type;
//     sb->sb_ops = sb_ops;
//     sb->ref_count = 1;
//     sb->private = NULL;
//
//     return sb;
// }
//
// void sb_destroy(super_block_t* sb) {
//     if (!sb) return;
//
//     // Call put_super if provided
//     if (sb->sb_ops && sb->sb_ops->put_super)
//         sb->sb_ops->put_super(sb);
//
//     kfree(sb);
// }
