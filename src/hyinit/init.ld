SECTIONS {
  .text 0x1000 : {
    *(.text)
    _etext = .;
    etext = _etext;
  }

}