#pragma once
#include <ctime>
#include <cstdint>

struct Partition {
    char part_status;       
    char part_type;         
    char part_fit;          
    char _pad;              
    int  part_start;        
    int  part_s;            
    char part_name[16];     
    int  part_correlative;  
    char part_id[4];        

    Partition() {
        part_status      = '0';
        part_type        = 'P';
        part_fit         = 'F';
        _pad             = 0;
        part_start       = -1;
        part_s           = 0;
        part_correlative = -1;
        for (int i = 0; i < 16; i++) part_name[i] = '\0';
        for (int i = 0; i <  4; i++) part_id[i]   = '\0';
    }
};

struct MBR {
    int       mbr_tamano;         
    time_t    mbr_fecha_creacion; 
    int       mbr_dsk_signature;  
    char      dsk_fit;            
    char      _pad[3];            
    Partition mbr_partitions[4]; 

    MBR() {
        mbr_tamano         = 0;
        mbr_fecha_creacion = 0;
        mbr_dsk_signature  = 0;
        dsk_fit            = 'F';
        for (int i = 0; i < 3; i++) _pad[i] = 0;
    }
};

struct EBR {
    char part_mount;    
    char part_fit;      
    char _pad[2];       
    int  part_start;    
    int  part_s;        
    int  part_next;     
    char part_name[16];

    EBR() {
        part_mount = '0';
        part_fit   = 'F';
        _pad[0] = _pad[1] = 0;
        part_start = -1;
        part_s     = 0;
        part_next  = -1;
        for (int i = 0; i < 16; i++) part_name[i] = '\0';
    }
};

struct SuperBlock {
    int    s_filesystem_type;   
    int    s_inodes_count;      
    int    s_blocks_count;      
    int    s_free_blocks_count; 
    int    s_free_inodes_count; 
    int    s_mtime;             
    int    s_umtime;            
    int    s_mnt_count;         
    int    s_magic;             
    int    s_inode_size;        
    int    s_block_size;        
    int    s_first_ino;         
    int    s_first_blo;         
    int    s_bm_inode_start;    
    int    s_bm_block_start;    
    int    s_inode_start;       
    int    s_block_start;      

    SuperBlock() {
        s_filesystem_type   = 2;
        s_inodes_count      = 0;
        s_blocks_count      = 0;
        s_free_blocks_count = 0;
        s_free_inodes_count = 0;
        s_mtime             = 0;
        s_umtime            = 0;
        s_mnt_count         = 0;
        s_magic             = 0xEF53;
        s_inode_size        = 0;
        s_block_size        = 0;
        s_first_ino         = 1;
        s_first_blo         = 0;
        s_bm_inode_start    = 0;
        s_bm_block_start    = 0;
        s_inode_start       = 0;
        s_block_start       = 0;
    }
};

struct Inode {
    int   i_uid;        
    int   i_gid;        
    int   i_size;       
    int   i_atime;      
    int   i_ctime;      
    int   i_mtime;      
    int   i_type;       
    int   i_perm;       
    int   i_block[15];  

    Inode() {
        i_uid  = 0;
        i_gid  = 0;
        i_size = 0;
        i_atime = i_ctime = i_mtime = 0;
        i_type = 0;
        i_perm = 664;
        for (int i = 0; i < 15; i++) i_block[i] = -1;
    }
};

struct FolderContent {
    int  inodo;      
    char name[12];   

    FolderContent() {
        inodo = -1;
        for (int i = 0; i < 12; i++) name[i] = '\0';
    }
};

struct FolderBlock {
    FolderContent b_content[4]; 
    FolderBlock() {}
};

struct FileBlock {
    char b_content[64]; 

    FileBlock() {
        for (int i = 0; i < 64; i++) b_content[i] = '\0';
    }
};

struct PointerBlock {
    int b_pointers[16]; 

    PointerBlock() {
        for (int i = 0; i < 16; i++) b_pointers[i] = -1;
    }
};
