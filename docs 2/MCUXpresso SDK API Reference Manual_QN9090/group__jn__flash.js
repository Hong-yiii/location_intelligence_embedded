var group__jn__flash =
[
    [ "IMG_HEADER_T", "group__jn__flash.html#structIMG__HEADER__T", [
      [ "vectors", "group__jn__flash.html#ga341195afcb2e1a972a8dcfbb7c77cad0", null ],
      [ "vectorCsum", "group__jn__flash.html#gaaf11554a0540465616aa0b7b900f1c8d", null ],
      [ "imageSignature", "group__jn__flash.html#ga5d34c7988190d3daf83479dc8d4309b0", null ],
      [ "bootBlockOffset", "group__jn__flash.html#ga6c5bcf549c91fdab2ac7db2b4a519a16", null ],
      [ "header_crc", "group__jn__flash.html#gae36c4f73648fde16e1767e5c770b6009", null ]
    ] ],
    [ "BOOT_BLOCK_T", "group__jn__flash.html#structBOOT__BLOCK__T", [
      [ "header_marker", "group__jn__flash.html#ga787414d119d0baa924153421785af7bb", null ],
      [ "img_type", "group__jn__flash.html#ga64de0b718841101149c1760666b632d3", null ],
      [ "target_addr", "group__jn__flash.html#ga91759e401b32d64553a721ea1ef79b6b", null ],
      [ "img_len", "group__jn__flash.html#ga223489eebc70d66b3497f048b3ab815b", null ],
      [ "stated_size", "group__jn__flash.html#gad766a69940a0277967b39fcdf5a05f2c", null ],
      [ "certificate_offset", "group__jn__flash.html#gafbbb904b835f023e0d7b014fe19bd608", null ],
      [ "compatibility_offset", "group__jn__flash.html#gaf039c881b4f8155bdb9ffa9670b8e738", null ],
      [ "version", "group__jn__flash.html#ga3f355e22b566ff3c3bf526e3bd71ce44", null ]
    ] ],
    [ "flash_config_t", "group__jn__flash.html#structflash__config__t", [
      [ "PFlashBlockBase", "group__jn__flash.html#a38d149791c84262f5526f278c250db6c", null ],
      [ "PFlashTotalSize", "group__jn__flash.html#a3d19cd0bb2c4f30c3a0e1a46400f9184", null ],
      [ "PFlashSectorSize", "group__jn__flash.html#ae9bec460d5fd4b27c7e4e7096036eebd", null ]
    ] ],
    [ "FLASH_FAIL", "group__jn__flash.html#ga2cf499effe92cb55c754475e6630aef3", null ],
    [ "FLASH_FAIL", "group__jn__flash.html#ga2cf499effe92cb55c754475e6630aef3", null ],
    [ "FLASH_ERR", "group__jn__flash.html#gae42c25def8c3dd47ecb26ff8514458de", null ],
    [ "FLASH_ERR", "group__jn__flash.html#gae42c25def8c3dd47ecb26ff8514458de", null ],
    [ "FLASH_DONE", "group__jn__flash.html#gaf8e68c6a269147a4528d1083632e7b15", null ],
    [ "FLASH_DONE", "group__jn__flash.html#gaf8e68c6a269147a4528d1083632e7b15", null ],
    [ "FLASH_ECC_ERR", "group__jn__flash.html#ga79c53e751493dd6727a60dc0e19bfeaf", null ],
    [ "FLASH_ECC_ERR", "group__jn__flash.html#ga79c53e751493dd6727a60dc0e19bfeaf", null ],
    [ "flash_status_t", "group__jn__flash.html#gab551389322a209cca1dcc1a7b2440f7a", [
      [ "kStatus_FLASH_Success", "group__jn__flash.html#ggab551389322a209cca1dcc1a7b2440f7aaacee5186bbe9db2e091dc5c36531cd7c", null ],
      [ "kStatus_FLASH_Fail", "group__jn__flash.html#ggab551389322a209cca1dcc1a7b2440f7aa42d8c5bdbd4503abcaa300b9dc1da580", null ],
      [ "kStatus_FLASH_InvalidArgument", "group__jn__flash.html#ggab551389322a209cca1dcc1a7b2440f7aa85c375f1213ec36dd2cf60e8d9851867", null ],
      [ "kStatus_FLASH_AlignmentError", "group__jn__flash.html#ggab551389322a209cca1dcc1a7b2440f7aa098148af25679d78133103cace7580ea", null ],
      [ "kStatus_FLASH_EccError", "group__jn__flash.html#ggab551389322a209cca1dcc1a7b2440f7aae61db800e78c6aa18cb5ecafe60058fc", null ],
      [ "kStatus_FLASH_Error", "group__jn__flash.html#ggab551389322a209cca1dcc1a7b2440f7aaae68794d392fef048c77ca503cc9bee3", null ]
    ] ],
    [ "flash_read_mode_t", "group__jn__flash.html#ga170e0beff9eb3794b98186b8cddca554", [
      [ "FLASH_ReadModeNormalEccOff", "group__jn__flash.html#gga170e0beff9eb3794b98186b8cddca554a4d7a2c12f6296af67dd42d13beb4fafc", null ]
    ] ],
    [ "FLASH_Init", "group__jn__flash.html#gaa6f605da4376485778af34202bcf4b86", null ],
    [ "FLASH_Powerdown", "group__jn__flash.html#ga456d9d5ea0c54f5560594192ab08313c", null ],
    [ "FLASH_Wait", "group__jn__flash.html#ga439424cc9bbb142eb3e51065833a0be8", null ],
    [ "FLASH_Erase", "group__jn__flash.html#ga9b02576ecf095f2162f78cd7359ed59d", null ],
    [ "FLASH_ErasePages", "group__jn__flash.html#ga7204760475e607519a0a6ad8169e8bc4", null ],
    [ "FLASH_BlankCheck", "group__jn__flash.html#gaa16ae891e3a2049bb76b15f9d2cca642", null ],
    [ "FLASH_MarginCheck", "group__jn__flash.html#ga3fc35c716d161c90b1350419eab03c7c", null ],
    [ "FLASH_Program", "group__jn__flash.html#ga4e86eba0e9e565809dac680d9c236cec", null ],
    [ "FLASH_Checksum", "group__jn__flash.html#ga6781c2a487d24e0f163a49deace82873", null ],
    [ "FLASH_Read", "group__jn__flash.html#ga4611b455300162912e176e069d36cbb7", null ],
    [ "FLASH_SetReadMode", "group__jn__flash.html#ga8856194d61f20e5f382f43504013252f", null ],
    [ "FLASH_CalculateChecksum", "group__jn__flash.html#ga05f32b1e3cdc0514636522532bf8e24e", null ],
    [ "FLASH_ConfigPageVerifyPageChecksum", "group__jn__flash.html#ga4862f7651cdebb6e7eab35c39dfc2d1e", null ],
    [ "FLASH_ConfigPageVerifyGpoChecksum", "group__jn__flash.html#ga67e569f3597f56c575327e94dca33bc8", null ],
    [ "FLASH_ConfigPageUpdate", "group__jn__flash.html#gae9d9cdd1d36911bf4c2f1a1869123031", null ],
    [ "FLASH_GetStatus", "group__jn__flash.html#ga3008ec421f72c29fddaf6ed5ad983983", null ],
    [ "vectors", "group__jn__flash.html#ga341195afcb2e1a972a8dcfbb7c77cad0", null ],
    [ "vectorCsum", "group__jn__flash.html#gaaf11554a0540465616aa0b7b900f1c8d", null ],
    [ "imageSignature", "group__jn__flash.html#ga5d34c7988190d3daf83479dc8d4309b0", null ],
    [ "bootBlockOffset", "group__jn__flash.html#ga6c5bcf549c91fdab2ac7db2b4a519a16", null ],
    [ "header_crc", "group__jn__flash.html#gae36c4f73648fde16e1767e5c770b6009", null ],
    [ "header_marker", "group__jn__flash.html#ga787414d119d0baa924153421785af7bb", null ],
    [ "img_type", "group__jn__flash.html#ga64de0b718841101149c1760666b632d3", null ],
    [ "target_addr", "group__jn__flash.html#ga91759e401b32d64553a721ea1ef79b6b", null ],
    [ "img_len", "group__jn__flash.html#ga223489eebc70d66b3497f048b3ab815b", null ],
    [ "stated_size", "group__jn__flash.html#gad766a69940a0277967b39fcdf5a05f2c", null ],
    [ "certificate_offset", "group__jn__flash.html#gafbbb904b835f023e0d7b014fe19bd608", null ],
    [ "compatibility_offset", "group__jn__flash.html#gaf039c881b4f8155bdb9ffa9670b8e738", null ],
    [ "version", "group__jn__flash.html#ga3f355e22b566ff3c3bf526e3bd71ce44", null ]
];