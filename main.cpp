// Yaniv Gilad
// 315641142

#include <iostream>
#include <vector>
#include <map>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

#define DISK_SIZE 256

void decToBinary(int n, char &c)
{
    // array to store binary number
    int binaryNum[8];

    // counter for binary array
    int i = 0;
    while (n > 0)
    {
        // storing remainder in binary array
        binaryNum[i] = n % 2;
        n = n / 2;
        i++;
    }

    // printing binary array in reverse order
    for (int j = i - 1; j >= 0; j--)
    {
        if (binaryNum[j] == 1)
            c = c | 1u << j;
    }
}

// #define SYS_CALL

// ============================================================================
class fsInode
{
    int fileSize;
    int block_in_use;

    int *directBlocks;
    int singleInDirect;
    int num_of_direct_blocks;
    int block_size;

public:
    fsInode(int _block_size, int _num_of_direct_blocks)
    {
        fileSize = 0;
        block_in_use = 0;
        block_size = _block_size;
        num_of_direct_blocks = _num_of_direct_blocks;
        directBlocks = new int[num_of_direct_blocks];
        assert(directBlocks);

        for (int i = 0; i < num_of_direct_blocks; i++)
        {
            directBlocks[i] = -1;
        }
        singleInDirect = -1;
    }

    // YOUR CODE......
    int get_file_size()
    {
        return fileSize;
    }

    int get_free_mem()
    {
        int max_size = (num_of_direct_blocks + block_size) * block_size;
        return max_size - fileSize;
    }

    int get_num_of_blocks_in_use()
    {
        return block_in_use;
    }

    int *get_direct_blocks_arr()
    {
        return directBlocks;
    }

    int get_indirect_block_num()
    {
        return singleInDirect;
    }

    void add_to_file_size(int num)
    {
        fileSize = fileSize + num;
    }

    int set_direct_block(int block_num)
    {
        if (block_in_use >= num_of_direct_blocks + block_size) // if there is not memory
        {
            return -1;
        }

        if (block_in_use < num_of_direct_blocks) // if there is free memory in direct blocks
        {
            directBlocks[block_in_use] = block_num;
            block_in_use++;
            return 1;
        }
    }

    int set_indirect_block(int num)
    {
        if (singleInDirect == -1)
        {
            singleInDirect = num;
            return 1;
        }
        return -1;
    }

    void inc_blocks_num()
    {
        block_in_use++;
    }

    ~fsInode()
    {
        free(directBlocks);
    }
};

// ============================================================================
class FileDescriptor
{
    pair<string, fsInode *> file;
    bool inUse;

public:
    FileDescriptor(string FileName, fsInode *fsi)
    {
        file.first = FileName;
        file.second = fsi;
        inUse = true;
    }

    string getFileName()
    {
        return file.first;
    }

    fsInode *getInode()
    {
        return file.second;
    }

    bool isInUse()
    {
        return (inUse);
    }
    void setInUse(bool _inUse)
    {
        inUse = _inUse;
    }

    void clear_name()
    {
        file.first = "\0";
    }
};

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"
// ============================================================================
class fsDisk
{
    FILE *sim_disk_fd;

    bool is_formated;

    // BitVector - "bit" (int) vector, indicate which block in the disk is free
    //              or not.  (i.e. if BitVector[0] == 1 , means that the
    //             first block is occupied.
    int BitVectorSize;
    int *BitVector;

    // Unix directories are lists of association structures,
    // each of which contains one filename and one inode number.
    map<string, fsInode *> MainDir;

    // OpenFileDescriptors --  when you open a file,
    // the operating system creates an entry to represent that file
    // This entry number is the file descriptor.
    vector<FileDescriptor> OpenFileDescriptors;

    int direct_enteris;
    int block_size;
    int num_of_blocks;
    int free_space;

    bool is_exist(string file_name) // check if file_name really in the map
    {
        if (is_formated == false)
            return false;

        map<string, fsInode *>::iterator itr;

        itr = MainDir.find(file_name);
        if (itr != MainDir.end()) // if there is already file with the same name
        {
            return true;
        }
        return false;
    }

    int get_free_block() // return free block (-1 if there is none)
    {
        for (int i = 0; i < BitVectorSize; i++)
        {
            if (BitVector[i] == 0)
            {
                BitVector[i] = 1;
                return i;
            }
        }

        return -1;
    }

    void write_to_disk(int fd, char *buf, int from, int to, int block) // write to disk
    {
        char temp[DISK_SIZE];
        lseek(fileno(sim_disk_fd), 0, SEEK_SET);
        if (read(fileno(sim_disk_fd), temp, DISK_SIZE) < DISK_SIZE)
        {
            perror("read error in write_to_disk");
            exit(1);
        }

        if (fd != -1)
        {
            for (int i = 0; i <= to - from; i++)
            {
                temp[block_size * block + i] = buf[from + i];
            }
        }

        else
        {
            int j = 0;
            for (int i = from; i <= to; i++)
            {
                temp[block_size * block + i] = buf[j];
                j++;
            }
        }

        ftruncate(fileno(sim_disk_fd), 0);
        lseek(fileno(sim_disk_fd), 0, SEEK_SET);
        if (write(fileno(sim_disk_fd), temp, DISK_SIZE) < DISK_SIZE)
        {
            perror("write error in write_to_disk");
            exit(1);
        }
    }

    int get_real_address(int indirect_block, int size)
    {
        char temp[DISK_SIZE];
        lseek(fileno(sim_disk_fd), 0, SEEK_SET);
        if (read(fileno(sim_disk_fd), temp, DISK_SIZE) < DISK_SIZE)
        {
            perror("read error in get_real_address");
            exit(1);
        }
        int current_block = size / block_size;
        int offset = current_block - direct_enteris;
        return (int)temp[block_size * indirect_block + offset];
    }

    void delete_block(int block) // delete block
    {
        char temp[DISK_SIZE];
        lseek(fileno(sim_disk_fd), 0, SEEK_SET);
        if (read(fileno(sim_disk_fd), temp, DISK_SIZE) < DISK_SIZE)
        {
            perror("read error in delete_block");
            exit(1);
        }

        for (int i = 0; i < block_size; i++)
        {
            temp[block_size * block + i] = '\0';
        }
        BitVector[block] = 0;

        ftruncate(fileno(sim_disk_fd), 0);
        lseek(fileno(sim_disk_fd), 0, SEEK_SET);
        if (write(fileno(sim_disk_fd), temp, DISK_SIZE) < DISK_SIZE)
        {
            perror("write error in write_to_disk");
            exit(1);
        }
    }

    int get_block_by_offset(int block, int offset) // return where the indirect blocks
    {
        char temp[DISK_SIZE];
        lseek(fileno(sim_disk_fd), 0, SEEK_SET);
        if (read(fileno(sim_disk_fd), temp, DISK_SIZE) < DISK_SIZE)
        {
            perror("read error in get_block_by_offset");
            exit(1);
        }
        return (int)temp[block * block_size + offset];
    }

    void write_char(char c, int place) // write char in specific place in disk
    {
        char temp[DISK_SIZE];
        lseek(fileno(sim_disk_fd), 0, SEEK_SET);
        if (read(fileno(sim_disk_fd), temp, DISK_SIZE) < DISK_SIZE)
        {
            perror("read error in write_char");
            exit(1);
        }

        temp[place] = c;

        ftruncate(fileno(sim_disk_fd), 0);
        lseek(fileno(sim_disk_fd), 0, SEEK_SET);
        if (write(fileno(sim_disk_fd), temp, DISK_SIZE) < DISK_SIZE)
        {
            perror("write error in write_to_disk");
            exit(1);
        }
    }

    char get_char(int place) // return char in specific place in disk
    {
        char temp[DISK_SIZE];
        lseek(fileno(sim_disk_fd), 0, SEEK_SET);
        if (read(fileno(sim_disk_fd), temp, DISK_SIZE) < DISK_SIZE)
        {
            perror("read error in get_char");
            exit(1);
        }

        return temp[place];
    }

    int how_many_free_blocks()
    {
        int count = 0;
        for (int i = 0; i < BitVectorSize; i++)
        {
            if (BitVector[i] == 0)
                count++;
        }
        return count;
    }

    int num_of_blocks_needed(int file_size, int frg, int len)
    {
        if (len <= 0)
            return 0;

        int b_taken = file_size / block_size;
        if (file_size % block_size > 0)
            b_taken++;

        int b_to_allocate = (len - frg) / block_size;
        if ((len - frg) % block_size > 0)
            b_to_allocate++;

        if (b_taken <= direct_enteris && (b_taken + b_to_allocate) > direct_enteris) // if need to allocate the indirect block
            b_to_allocate++;

        return b_to_allocate;
    }

public:
    // ------------------------------------------------------------------------
    fsDisk()
    {
        sim_disk_fd = fopen(DISK_SIM_FILE, "r+");
        ftruncate(fileno(sim_disk_fd), 0);
        assert(sim_disk_fd);
        for (int i = 0; i < DISK_SIZE; i++)
        {
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fwrite("\0", 1, 1, sim_disk_fd);
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);
        is_formated = false;
    }

    // ------------------------------------------------------------------------
    void listAll()
    {
        int i = 0;
        for (auto it = begin(OpenFileDescriptors); it != end(OpenFileDescriptors); ++it)
        {
            cout << "index: " << i << ": FileName: " << it->getFileName() << " , isInUse: " << it->isInUse() << endl;
            i++;
        }

        cout << "Disk content: '";
        char temp[DISK_SIZE];
        lseek(fileno(sim_disk_fd), 0, SEEK_SET);
        if (read(fileno(sim_disk_fd), temp, DISK_SIZE) < DISK_SIZE)
        {
            perror("read error in write_to_disk");
            exit(1);
        }
        for (i = 0; i < DISK_SIZE; i++)
        {
            if ((int)temp[i] > 32)
                cout << temp[i];
        }

        cout << "'" << endl;
    }

    // ------------------------------------------------------------------------
    void fsFormat(int blockSize = 4, int direct_Enteris_ = 3)
    {
        free_space = DISK_SIZE;
        direct_enteris = direct_Enteris_;
        block_size = blockSize;
        num_of_blocks = DISK_SIZE / block_size;
        is_formated = true;
        BitVectorSize = num_of_blocks;
        BitVector = (int *)malloc(sizeof(int) * BitVectorSize);
        assert(BitVector != NULL);
        for (int i = 0; i < BitVectorSize; i++)
            BitVector[i] = 0;

        cout << "FORMAT DISK: number of blocks: " << num_of_blocks << endl;
    }

    // ------------------------------------------------------------------------
    int CreateFile(string fileName)
    {
        if (is_formated == false)
            return -1;

        if (this->is_exist(fileName) == true)
            return -1;

        // *** create file *** //
        vector<FileDescriptor>::iterator place;

        FileDescriptor fd = FileDescriptor(fileName, new fsInode(block_size, direct_enteris));
        MainDir.insert(pair<string, fsInode *>(fd.getFileName(), fd.getInode()));

        place = OpenFileDescriptors.size() + OpenFileDescriptors.begin();
        OpenFileDescriptors.insert(place, 1, fd);
        return OpenFileDescriptors.size() - 1;
    }

    // ------------------------------------------------------------------------
    int OpenFile(string fileName)
    {
        if (is_formated == false)
            return -1;

        if (this->is_exist(fileName) == false)
            return -1;

        for (int i = 0; i < OpenFileDescriptors.size(); i++)
        {
            if (OpenFileDescriptors.at(i).getFileName().compare(fileName) == 0 && OpenFileDescriptors.at(i).isInUse() == false)
            {
                OpenFileDescriptors.at(i).setInUse(true);
                return i;
            }
        }

        return -1;
    }

    // ------------------------------------------------------------------------
    string CloseFile(int fd)
    {
        if (is_formated == false)
            return "-1";

        if (fd >= OpenFileDescriptors.size() || fd < 0)
            return "-1";

        if (OpenFileDescriptors.at(fd).isInUse() == true) // if it's really open
        {
            OpenFileDescriptors.at(fd).setInUse(false);
            return OpenFileDescriptors.at(fd).getFileName();
        }

        return "-1";
    }
    // ------------------------------------------------------------------------
    int WriteToFile(int fd, char *buf, int len)
    {
        // check valid input
        bool frg = false;
        int index = 0;
        int phyisical_block;
        if (fd >= OpenFileDescriptors.size() || fd < 0 || is_formated == false || len > free_space)
            return -1;

        FileDescriptor &file = OpenFileDescriptors.at(fd);
        if (file.isInUse() == false || file.getInode()->get_free_mem() < len || is_exist(file.getFileName()) == false)
            return -1;

        int file_size = file.getInode()->get_file_size();
        int *dir_blocks = file.getInode()->get_direct_blocks_arr();

        int current_block = file_size / block_size;
        int internal_frg = block_size - (file_size % block_size);

        // deal with fragmantation
        if (internal_frg > 0 && internal_frg < block_size && file_size != 0)
        {
            frg = true;
            if (num_of_blocks_needed(file_size, internal_frg, len) > how_many_free_blocks())
            {
                cout << "not enough blocks" << endl;
                return -1;
            }

            if (current_block < direct_enteris) // fragmantation in direct blocks
                phyisical_block = dir_blocks[current_block];

            else // fragmantation in **indirect** blocks
            {
                phyisical_block = file.getInode()->get_indirect_block_num();
                phyisical_block = this->get_real_address(phyisical_block, file_size);
            }

            int from = block_size - internal_frg;
            int to;
            if (len >= internal_frg)
                to = block_size - 1;
            else
                to = block_size - internal_frg + len - 1;
            write_to_disk(-1, buf, from, to, phyisical_block);
            free_space = free_space - (to - from) - 1;
            file.getInode()->add_to_file_size((to - from) + 1);
            current_block++;
            index = to - from + 1;
            internal_frg = 0;
        }

        if (frg == false && num_of_blocks_needed(file_size, 0, len) > how_many_free_blocks()) // not enough blocks
        {
            cout << "not enough blocks" << endl;
            return -1;
        }

        while (index < len)
        {
            if (current_block < direct_enteris) // there is still direct block
            {
                int b = get_free_block();
                if (b == -1)
                    return -1;
                dir_blocks[current_block] = b;
                if (len - index >= block_size) // full block
                {
                    write_to_disk(fd, buf, index, index + block_size - 1, b);
                    index += block_size;
                    free_space = free_space - block_size;
                    file.getInode()->add_to_file_size(block_size);
                    current_block++;
                }
                else
                {
                    write_to_disk(fd, buf, index, len - 1, b);
                    free_space = free_space - (len - index);
                    file.getInode()->add_to_file_size(len - index);
                    index = len;
                }
            }

            else // if current block is indirect
            {
                if (file.getInode()->get_indirect_block_num() == -1) // if there is no indirect block
                {
                    int b = get_free_block();
                    if (b == -1)
                        return -1;

                    file.getInode()->set_indirect_block(b);
                }

                int indirect = file.getInode()->get_indirect_block_num();
                int b = get_free_block();
                if (b == -1)
                    return -1;
                char c = '\0';
                decToBinary(b, c);

                int offset = current_block - direct_enteris;
                int place = indirect * block_size + offset;
                write_char(c, place);

                if (len - index >= block_size) // full block
                {
                    write_to_disk(fd, buf, index, index + block_size - 1, b);
                    index += block_size;
                    free_space = free_space - block_size;
                    file.getInode()->add_to_file_size(block_size);
                    current_block++;
                }
                else
                {
                    write_to_disk(fd, buf, index, len - 1, b);
                    free_space = free_space - (len - index);
                    file.getInode()->add_to_file_size(len - index);
                    index = len;
                }
            }
        }
        return 1;
    }

    // ------------------------------------------------------------------------
    int DelFile(string FileName)
    {
        if (is_exist(FileName) == false)
            return -1;

        fsInode *temp = MainDir.operator[](FileName);
        int size = temp->get_file_size();
        int fd = 0;
        for (int i = 0; i < OpenFileDescriptors.size(); i++) // find fd
        {
            if (OpenFileDescriptors.at(i).getFileName().compare(FileName) == 0)
            {
                OpenFileDescriptors.at(i).setInUse(false);
                OpenFileDescriptors.at(i).clear_name();
                fd = i;
            }
        }

        int *dir_arr = temp->get_direct_blocks_arr();
        int b;
        int num_of_blocks = size / block_size;
        for (int i = 0; i <= num_of_blocks; i++)
        {
            if (i < direct_enteris) // delete direct blocks
            {
                b = dir_arr[i];
                if (b != -1)
                    delete_block(b);
            }
            else // delete indirect blocks
            {
                int indirect = temp->get_indirect_block_num();
                if (indirect == -1)
                    return fd;
                int offset = i - direct_enteris;

                b = get_block_by_offset(indirect, offset);
                delete_block(b);
            }
        }
        free_space += size;

        if (temp->get_indirect_block_num() != -1)
            delete_block(temp->get_indirect_block_num());

        MainDir.at(FileName)->~fsInode();
        MainDir.erase(FileName);
        return fd;
    }
    // ------------------------------------------------------------------------
    int ReadFromFile(int fd, char *buf, int len)
    {
        for (int i = 0; i <= len; i++)
            buf[i] = '\0';

        if (fd >= OpenFileDescriptors.size() || fd < 0 || is_formated == false || len < 0)
            return -1;

        FileDescriptor &file = OpenFileDescriptors.at(fd);
        if (file.isInUse() == false || is_exist(file.getFileName()) == false)
            return -1;

        if (file.getInode()->get_file_size() < len)
            len = file.getInode()->get_file_size();

        int *dir_arr = file.getInode()->get_direct_blocks_arr();
        int b;
        int offset;
        for (int i = 0; i < len; i++)
        {
            b = i / block_size;
            offset = i % block_size;
            if (b < direct_enteris) // copy from direct blocks
            {
                b = dir_arr[b];
                if (b != -1)
                    buf[i] = get_char(b * block_size + offset);
            }

            else // copy from indirect blocks
            {
                int indirect = file.getInode()->get_indirect_block_num();
                if (indirect == -1)
                    return i;
                b = get_block_by_offset(indirect, b - direct_enteris);
                buf[i] = get_char(b * block_size + offset);
            }
        }
        return len;
    }

    ~fsDisk()
    {
        free(BitVector);
        fclose(sim_disk_fd);

        for (int i = 0; i < OpenFileDescriptors.size(); i++)
        {
            if (is_exist(OpenFileDescriptors.at(i).getFileName()))
                MainDir.at(OpenFileDescriptors.at(i).getFileName())->~fsInode();
            free(OpenFileDescriptors.at(i).getInode());
        }
    }
};

int main()
{

    int blockSize;
    int direct_entries;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read;
    int _fd;

    fsDisk *fs = new fsDisk();
    int cmd_;
    while (1)
    {
        cin >> cmd_;
        switch (cmd_)
        {
        case 0: // exit
            delete fs;
            exit(0);
            break;

        case 1: // list-file
            fs->listAll();
            break;

        case 2: // format
            cin >> blockSize;
            cin >> direct_entries;
            fs->fsFormat(blockSize, direct_entries);
            break;

        case 3: // creat-file
            cin >> fileName;
            _fd = fs->CreateFile(fileName);
            cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;

        case 4: // open-file
            cin >> fileName;
            _fd = fs->OpenFile(fileName);
            cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;

        case 5: // close-file
            cin >> _fd;
            fileName = fs->CloseFile(_fd);
            cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;

        case 6: // write-file
            cin >> _fd;
            cin >> str_to_write;
            fs->WriteToFile(_fd, str_to_write, strlen(str_to_write));
            break;

        case 7: // read-file
            cin >> _fd;
            cin >> size_to_read;
            fs->ReadFromFile(_fd, str_to_read, size_to_read);
            cout << "ReadFromFile: " << str_to_read << endl;
            break;

        case 8: // delete file
            cin >> fileName;
            _fd = fs->DelFile(fileName);
            cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;
        default:
            break;
        }
    }
}