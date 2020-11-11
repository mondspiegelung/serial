#ifndef FILE_DESCRIPTOR_H
#define FILE_DESCRIPTOR_H 1

namespace util {

class file_descriptor
{
 public:
	file_descriptor();

	file_descriptor(int fd);

	file_descriptor(const file_descriptor &) = delete;

	file_descriptor(file_descriptor && other);

	file_descriptor & operator = (const file_descriptor &) = delete;

	file_descriptor & operator = (file_descriptor && other);

	virtual ~file_descriptor();

	void close();

	operator int() const;

 protected:
	int fd;
};

} // namespace util

#endif // FILE_DESCRIPTOR_H
