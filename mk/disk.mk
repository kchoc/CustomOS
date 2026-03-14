$(DISK):
	truncate -s $(IMAGE_SIZE_MB)M $@
	mformat -i "$@@@2048S" -H 2048 ::
	mmd -i "$@@@2048S" ::/bin
	mmd -i "$@@@2048S" ::/boot
	mmd -i "$@@@2048S" ::/dev
	mmd -i "$@@@2048S" ::/etc
	mmd -i "$@@@2048S" ::/home
	mmd -i "$@@@2048S" ::/lib
	mmd -i "$@@@2048S" ::/media
	mmd -i "$@@@2048S" ::/mnt
	mmd -i "$@@@2048S" ::/opt
	mmd -i "$@@@2048S" ::/proc
	mmd -i "$@@@2048S" ::/root
	mmd -i "$@@@2048S" ::/run
	mmd -i "$@@@2048S" ::/sbin
	mmd -i "$@@@2048S" ::/srv
	mmd -i "$@@@2048S" ::/sys
	mmd -i "$@@@2048S" ::/tmp
	mmd -i "$@@@2048S" ::/usr
	mmd -i "$@@@2048S" ::/var
	
