var group__spifi =
[
    [ "SPIFI DMA Driver", "group__spifi__dma__driver.html", null ],
    [ "SPIFI Driver", "group__spifi__driver.html", null ],
    [ "spifi_command_t", "group__spifi.html#structspifi__command__t", [
      [ "dataLen", "group__spifi.html#acdb8a7c713e1abd3950a78c807b8ca49", null ],
      [ "isPollMode", "group__spifi.html#a0eff84aeb7d8ef6513696dc673b7c185", null ],
      [ "direction", "group__spifi.html#ab9add9001cf8b0b0c469ec130b4b4bf9", null ],
      [ "intermediateBytes", "group__spifi.html#a0aed2cfb84d226015863cd454a74ee70", null ],
      [ "format", "group__spifi.html#ab2963e3ec49fca32f9a2239b57818ad3", null ],
      [ "type", "group__spifi.html#adc67dd85c6bb04e09b5b878198cb006c", null ],
      [ "opcode", "group__spifi.html#a07e4fd425c66698881336ebb74c7127c", null ]
    ] ],
    [ "spifi_config_t", "group__spifi.html#structspifi__config__t", [
      [ "timeout", "group__spifi.html#a49b406925e95a80e4feca030b19033e4", null ],
      [ "csHighTime", "group__spifi.html#a446d0d71c68f1f1f05ec806fbab0c8ec", null ],
      [ "disablePrefetch", "group__spifi.html#a95d7f1144d316bb2291608f9c7f1bac1", null ],
      [ "disableCachePrefech", "group__spifi.html#a98296da3429c41b069f8943822945121", null ],
      [ "isFeedbackClock", "group__spifi.html#a6b372c9f5558cdff5f6d9c5d2ebb9c36", null ],
      [ "spiMode", "group__spifi.html#a19890c115fee36b391fc9d50cde504a0", null ],
      [ "isReadFullClockCycle", "group__spifi.html#ababc2b10f8ad3a6a6703de6ba06af639", null ],
      [ "dualMode", "group__spifi.html#a11e48be5ec4492d4fb99b740988393a0", null ]
    ] ],
    [ "spifi_transfer_t", "group__spifi.html#structspifi__transfer__t", [
      [ "data", "group__spifi.html#ae13d7340f8a0fee0c3e373165834fe0c", null ],
      [ "dataSize", "group__spifi.html#a6327b971b924d3233d54b7a4fa7a20a2", null ]
    ] ],
    [ "spifi_dma_handle_t", "group__spifi.html#struct__spifi__dma__handle", [
      [ "dmaHandle", "group__spifi.html#ad59c277113842f6542337c38d655950e", null ],
      [ "transferSize", "group__spifi.html#aa11d1634272e143aafede47be4bdd8fe", null ],
      [ "state", "group__spifi.html#ad885c55eb8f16bc42322cfe2fd953a6c", null ],
      [ "callback", "group__spifi.html#a9a73928e330b31026f70cc80cbbca062", null ],
      [ "userData", "group__spifi.html#acfb5c8239aa46ed67a18321fe6b85db8", null ]
    ] ],
    [ "FSL_SPIFI_DRIVER_VERSION", "group__spifi.html#ga8a3aaee54a41ac3671f0991ef0be4311", null ],
    [ "FSL_SPIFI_DMA_DRIVER_VERSION", "group__spifi.html#ga88cab5070960fa75d27cb3d9ae9a653f", null ],
    [ "spifi_dma_callback_t", "group__spifi.html#ga1653912b903b8e56fd3a59b8aeff56e2", null ],
    [ "_status_t", "group__spifi.html#ga9006d37909032f8ad238cb8f953115a4", [
      [ "kStatus_SPIFI_Idle", "group__spifi.html#gga9006d37909032f8ad238cb8f953115a4a1101ca947feca751614b60d5f0e564a6", null ],
      [ "kStatus_SPIFI_Busy", "group__spifi.html#gga9006d37909032f8ad238cb8f953115a4a0c05b2ed19fbc1f52bef452bfd6f3a7e", null ],
      [ "kStatus_SPIFI_Error", "group__spifi.html#gga9006d37909032f8ad238cb8f953115a4afb274a0714b707188be3c5455f4ae256", null ]
    ] ],
    [ "spifi_interrupt_enable_t", "group__spifi.html#ga394407d233f3b5285b653c4476037922", [
      [ "kSPIFI_CommandFinishInterruptEnable", "group__spifi.html#gga394407d233f3b5285b653c4476037922aafda0f615d4250e87903d792c5332bec", null ]
    ] ],
    [ "spifi_spi_mode_t", "group__spifi.html#ga2068868ffe1d58bd004331e5bde2759a", [
      [ "kSPIFI_SPISckLow", "group__spifi.html#gga2068868ffe1d58bd004331e5bde2759aa54a94b95480979dec1c0be7886b34a9b", null ],
      [ "kSPIFI_SPISckHigh", "group__spifi.html#gga2068868ffe1d58bd004331e5bde2759aac7d35eb1ba9c1831bbe3fdbcb7369c2f", null ]
    ] ],
    [ "spifi_dual_mode_t", "group__spifi.html#ga8b0d8d9478f4e192547093dc50ef1112", [
      [ "kSPIFI_QuadMode", "group__spifi.html#gga8b0d8d9478f4e192547093dc50ef1112a1695564d14b417451bf2e20013b3bf9c", null ],
      [ "kSPIFI_DualMode", "group__spifi.html#gga8b0d8d9478f4e192547093dc50ef1112aa4f8a3b1b4146ac35eb1d8713a67003e", null ]
    ] ],
    [ "spifi_data_direction_t", "group__spifi.html#gab5045fc8464d34d61ad79eb5052213a2", [
      [ "kSPIFI_DataInput", "group__spifi.html#ggab5045fc8464d34d61ad79eb5052213a2aa2aaf75e95c68df197e026c4d700d8ec", null ],
      [ "kSPIFI_DataOutput", "group__spifi.html#ggab5045fc8464d34d61ad79eb5052213a2ab5a25428136bc39f47964449770d3171", null ]
    ] ],
    [ "spifi_command_format_t", "group__spifi.html#ga6bf3d9310b87c8b483e218e1c89835d8", [
      [ "kSPIFI_CommandAllSerial", "group__spifi.html#gga6bf3d9310b87c8b483e218e1c89835d8a285650c0dca27852c72aa3de378cb007", null ],
      [ "kSPIFI_CommandDataQuad", "group__spifi.html#gga6bf3d9310b87c8b483e218e1c89835d8a147356148985bc6dbc07d09ca2e4e17d", null ],
      [ "kSPIFI_CommandOpcodeSerial", "group__spifi.html#gga6bf3d9310b87c8b483e218e1c89835d8a3a3f3fe40218761a04c4bc0475481623", null ],
      [ "kSPIFI_CommandAllQuad", "group__spifi.html#gga6bf3d9310b87c8b483e218e1c89835d8ab76224bb76a944c75e679bb618c87acc", null ]
    ] ],
    [ "spifi_command_type_t", "group__spifi.html#gaa9c7a179b69736234f93fdf8f4dec12d", [
      [ "kSPIFI_CommandOpcodeOnly", "group__spifi.html#ggaa9c7a179b69736234f93fdf8f4dec12dac53c116f951ebac1581fb101b9a089ed", null ],
      [ "kSPIFI_CommandOpcodeAddrOneByte", "group__spifi.html#ggaa9c7a179b69736234f93fdf8f4dec12dadccedc9bf002106b14d1aef2e5624e7c", null ],
      [ "kSPIFI_CommandOpcodeAddrTwoBytes", "group__spifi.html#ggaa9c7a179b69736234f93fdf8f4dec12dab69cb3087f091f98b8edcea994689e94", null ],
      [ "kSPIFI_CommandOpcodeAddrThreeBytes", "group__spifi.html#ggaa9c7a179b69736234f93fdf8f4dec12daf6ad368e7c54448e4faf08215e919ef4", null ],
      [ "kSPIFI_CommandOpcodeAddrFourBytes", "group__spifi.html#ggaa9c7a179b69736234f93fdf8f4dec12dab7d2ea2b5ba591a33c8dee7fd4df7ec3", null ],
      [ "kSPIFI_CommandNoOpcodeAddrThreeBytes", "group__spifi.html#ggaa9c7a179b69736234f93fdf8f4dec12da3148a393cd069218c87d3beecb98c201", null ],
      [ "kSPIFI_CommandNoOpcodeAddrFourBytes", "group__spifi.html#ggaa9c7a179b69736234f93fdf8f4dec12da737414e798fb23ba0db56081523839fb", null ]
    ] ],
    [ "_spifi_status_flags", "group__spifi.html#ga911493a6bdc330364e1fa7e56de3a5fc", [
      [ "kSPIFI_MemoryCommandWriteFinished", "group__spifi.html#gga911493a6bdc330364e1fa7e56de3a5fcaca385bc1eee3fe16737230af1d282ad9", null ],
      [ "kSPIFI_CommandWriteFinished", "group__spifi.html#gga911493a6bdc330364e1fa7e56de3a5fca075b13c95c106d1d33ebd2d155fb6886", null ],
      [ "kSPIFI_InterruptRequest", "group__spifi.html#gga911493a6bdc330364e1fa7e56de3a5fca75137dc285b383dbd29ea30082f31292", null ]
    ] ],
    [ "SPIFI_GetInstance", "group__spifi.html#ga5ae90915e8ba150b8bc808b79bfd165c", null ],
    [ "SPIFI_Init", "group__spifi.html#ga004433b0ee8eab70c8c97e0b817c1bc6", null ],
    [ "SPIFI_GetDefaultConfig", "group__spifi.html#ga91aa10cb7be3fcec3a736b7250e6bf56", null ],
    [ "SPIFI_Deinit", "group__spifi.html#ga76101ec5e4502b0f0a0d4d7c18ed5000", null ],
    [ "SPIFI_SetCommand", "group__spifi.html#gaa6f20a825c027c03cbf5513074578ff8", null ],
    [ "SPIFI_SetCommandAddress", "group__spifi.html#ga3bde2b8deef94d2ce53a46cc94c0a6f1", null ],
    [ "SPIFI_SetIntermediateData", "group__spifi.html#ga72601eeed840f46603bf870ca4797791", null ],
    [ "SPIFI_SetCacheLimit", "group__spifi.html#gaee68535940cf84012952bc08de072b04", null ],
    [ "SPIFI_ResetCommand", "group__spifi.html#ga97a286615172dda6a3f95e2a146536a2", null ],
    [ "SPIFI_SetMemoryCommand", "group__spifi.html#ga4305348460de12413e1c46b0dc0fb54d", null ],
    [ "SPIFI_EnableInterrupt", "group__spifi.html#gab278f07d73a428ff51cbcb5515869d6b", null ],
    [ "SPIFI_DisableInterrupt", "group__spifi.html#gacac402c429a579f31faaca2d008e3f98", null ],
    [ "SPIFI_GetStatusFlag", "group__spifi.html#ga62b10ead53813c1985acce1d35e837d7", null ],
    [ "SPIFI_EnableDMA", "group__spifi.html#gadfb055260d3d56950f7f758d51bf33a6", null ],
    [ "SPIFI_GetDataRegisterAddress", "group__spifi.html#ga9b7ecdafc39b6a1c9cea0d13e6f1a860", null ],
    [ "SPIFI_WriteData", "group__spifi.html#ga609232211dfc5ea09e464ec4ed0448ce", null ],
    [ "SPIFI_WriteBuffer", "group__spifi.html#gac7248fbd35be3c920e0265f2f1449ee8", null ],
    [ "SPIFI_WriteDataByte", "group__spifi.html#ga0960546c0dbf4c7b5d13d635cc8e08ed", null ],
    [ "SPIFI_WriteDataHalfword", "group__spifi.html#gad8c230154c38b8ba26f2d2053fb6062c", null ],
    [ "SPIFI_ReadData", "group__spifi.html#gad28658c5f4ce42c997349946e2ff0c18", null ],
    [ "SPIFI_ReadDataByte", "group__spifi.html#ga4846d7d3bb877f7f7a1614ec3f733075", null ],
    [ "SPIFI_ReadDataHalfword", "group__spifi.html#gabee164e27daae18ff52feb822a192b95", null ],
    [ "SPIFI_TransferTxCreateHandleDMA", "group__spifi.html#gafc88c240ef57a2955369f994cc1ca1d7", null ],
    [ "SPIFI_TransferRxCreateHandleDMA", "group__spifi.html#ga1da9cace8fae019d6d5adc08312a92d2", null ],
    [ "SPIFI_TransferSendDMA", "group__spifi.html#ga06493db0912e6538f2d2853253917ab3", null ],
    [ "SPIFI_TransferReceiveDMA", "group__spifi.html#gafd6e07368c22af51aac4e83c785ef88b", null ],
    [ "SPIFI_TransferAbortSendDMA", "group__spifi.html#ga1bae80a3b699c0cd59f94491d92aae3c", null ],
    [ "SPIFI_TransferAbortReceiveDMA", "group__spifi.html#ga2bb020c333a6195353089ac2325a94b0", null ],
    [ "SPIFI_TransferGetSendCountDMA", "group__spifi.html#gaf0ff9d9d872e7ea341cd124763cc9c5f", null ],
    [ "SPIFI_TransferGetReceiveCountDMA", "group__spifi.html#ga5170793d9e1ede2f8a273e376229c712", null ]
];