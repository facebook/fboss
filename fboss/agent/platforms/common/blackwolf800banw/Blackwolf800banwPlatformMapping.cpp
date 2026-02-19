/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/blackwolf800banw/Blackwolf800banwPlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{
  "ports": {
    "17": {
        "mapping": {
          "id": 17,
          "name": "rcy1/1/529",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_RCY-slot1/chip1/core529",
                "lane": 0
              }
            }
          ],
          "portType": 3,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 3,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "49": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_RCY-slot1/chip1/core529",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "18": {
        "mapping": {
          "id": 18,
          "name": "rcy1/1/530",
          "controllingPort": 18,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_RCY-slot1/chip1/core530",
                "lane": 0
              }
            }
          ],
          "portType": 3,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 4,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "49": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_RCY-slot1/chip1/core530",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "19": {
        "mapping": {
          "id": 19,
          "name": "rcy1/1/531",
          "controllingPort": 19,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_RCY-slot1/chip1/core531",
                "lane": 0
              }
            }
          ],
          "portType": 3,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 2,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "49": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_RCY-slot1/chip1/core531",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "20": {
        "mapping": {
          "id": 20,
          "name": "rcy1/1/532",
          "controllingPort": 20,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_RCY-slot1/chip1/core532",
                "lane": 0
              }
            }
          ],
          "portType": 3,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 3,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "49": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_RCY-slot1/chip1/core532",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "21": {
        "mapping": {
          "id": 21,
          "name": "rcy1/1/533",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_RCY-slot1/chip1/core533",
                "lane": 0
              }
            }
          ],
          "portType": 3,
          "attachedCoreId": 2,
          "attachedCorePortIndex": 2,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "49": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_RCY-slot1/chip1/core533",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "22": {
        "mapping": {
          "id": 22,
          "name": "rcy1/1/534",
          "controllingPort": 22,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_RCY-slot1/chip1/core534",
                "lane": 0
              }
            }
          ],
          "portType": 3,
          "attachedCoreId": 2,
          "attachedCorePortIndex": 3,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "49": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_RCY-slot1/chip1/core534",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "23": {
        "mapping": {
          "id": 23,
          "name": "rcy1/1/535",
          "controllingPort": 23,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_RCY-slot1/chip1/core535",
                "lane": 0
              }
            }
          ],
          "portType": 3,
          "attachedCoreId": 3,
          "attachedCorePortIndex": 2,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "49": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_RCY-slot1/chip1/core535",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "24": {
        "mapping": {
          "id": 24,
          "name": "rcy1/1/536",
          "controllingPort": 24,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_RCY-slot1/chip1/core536",
                "lane": 0
              }
            }
          ],
          "portType": 3,
          "attachedCoreId": 3,
          "attachedCorePortIndex": 3,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "49": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_RCY-slot1/chip1/core536",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "25": {
        "mapping": {
          "id": 25,
          "name": "rcy1/1/537",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_RCY-slot1/chip1/core537",
                "lane": 0
              }
            }
          ],
          "portType": 3,
          "attachedCoreId": 4,
          "attachedCorePortIndex": 2,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "49": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_RCY-slot1/chip1/core537",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "26": {
        "mapping": {
          "id": 26,
          "name": "rcy1/1/538",
          "controllingPort": 26,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_RCY-slot1/chip1/core538",
                "lane": 0
              }
            }
          ],
          "portType": 3,
          "attachedCoreId": 4,
          "attachedCorePortIndex": 3,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "49": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_RCY-slot1/chip1/core538",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "27": {
        "mapping": {
          "id": 27,
          "name": "rcy1/1/539",
          "controllingPort": 27,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_RCY-slot1/chip1/core539",
                "lane": 0
              }
            }
          ],
          "portType": 3,
          "attachedCoreId": 5,
          "attachedCorePortIndex": 2,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "49": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_RCY-slot1/chip1/core539",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "28": {
        "mapping": {
          "id": 28,
          "name": "rcy1/1/540",
          "controllingPort": 28,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_RCY-slot1/chip1/core540",
                "lane": 0
              }
            }
          ],
          "portType": 3,
          "attachedCoreId": 5,
          "attachedCorePortIndex": 3,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "49": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_RCY-slot1/chip1/core540",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "29": {
        "mapping": {
          "id": 29,
          "name": "rcy1/1/541",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_RCY-slot1/chip1/core541",
                "lane": 0
              }
            }
          ],
          "portType": 3,
          "attachedCoreId": 6,
          "attachedCorePortIndex": 2,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "49": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_RCY-slot1/chip1/core541",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "30": {
        "mapping": {
          "id": 30,
          "name": "rcy1/1/542",
          "controllingPort": 30,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_RCY-slot1/chip1/core542",
                "lane": 0
              }
            }
          ],
          "portType": 3,
          "attachedCoreId": 6,
          "attachedCorePortIndex": 3,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "49": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_RCY-slot1/chip1/core542",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "31": {
        "mapping": {
          "id": 31,
          "name": "rcy1/1/543",
          "controllingPort": 31,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_RCY-slot1/chip1/core543",
                "lane": 0
              }
            }
          ],
          "portType": 3,
          "attachedCoreId": 7,
          "attachedCorePortIndex": 2,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "49": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_RCY-slot1/chip1/core543",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "32": {
        "mapping": {
          "id": 32,
          "name": "rcy1/1/544",
          "controllingPort": 32,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_RCY-slot1/chip1/core544",
                "lane": 0
              }
            }
          ],
          "portType": 3,
          "attachedCoreId": 7,
          "attachedCorePortIndex": 3,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "49": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_RCY-slot1/chip1/core544",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "33": {
        "mapping": {
          "id": 33,
          "name": "evt1/1/1033",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_EVT-slot1/chip1/core1033",
                "lane": 0
              }
            }
          ],
          "portType": 5,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 5,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "49": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_EVT-slot1/chip1/core1033",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "34": {
        "mapping": {
          "id": 34,
          "name": "evt1/1/1034",
          "controllingPort": 34,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_EVT-slot1/chip1/core1034",
                "lane": 0
              }
            }
          ],
          "portType": 5,
          "attachedCoreId": 4,
          "attachedCorePortIndex": 4,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "49": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_EVT-slot1/chip1/core1034",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "35": {
        "mapping": {
          "id": 35,
          "name": "eth1/49/1",
          "controllingPort": 35,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 6,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core0",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip49",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "43": {
        "mapping": {
          "id": 43,
          "name": "eth1/51/1",
          "controllingPort": 43,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 8,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core1",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip51",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "51": {
        "mapping": {
          "id": 51,
          "name": "eth1/52/1",
          "controllingPort": 51,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 9,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core2",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip52",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "59": {
        "mapping": {
          "id": 59,
          "name": "eth1/54/1",
          "controllingPort": 59,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 11,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core3",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip54",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "67": {
        "mapping": {
          "id": 67,
          "name": "eth1/53/1",
          "controllingPort": 67,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 10,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core4",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip53",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "75": {
        "mapping": {
          "id": 75,
          "name": "eth1/55/1",
          "controllingPort": 75,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 12,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core5",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip55",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "83": {
        "mapping": {
          "id": 83,
          "name": "eth1/50/1",
          "controllingPort": 83,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 7,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core6",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip50",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "91": {
        "mapping": {
          "id": 91,
          "name": "eth1/56/1",
          "controllingPort": 91,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 0,
          "attachedCorePortIndex": 13,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core7",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip56",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "99": {
        "mapping": {
          "id": 99,
          "name": "eth1/58/1",
          "controllingPort": 99,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 5,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core8",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip58",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "107": {
        "mapping": {
          "id": 107,
          "name": "eth1/59/1",
          "controllingPort": 107,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 6,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core9",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip59",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "115": {
        "mapping": {
          "id": 115,
          "name": "eth1/62/1",
          "controllingPort": 115,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 9,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core10",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip62",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "123": {
        "mapping": {
          "id": 123,
          "name": "eth1/61/1",
          "controllingPort": 123,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 8,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core11",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip61",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "131": {
        "mapping": {
          "id": 131,
          "name": "eth1/64/1",
          "controllingPort": 131,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 11,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core12",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip64",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "139": {
        "mapping": {
          "id": 139,
          "name": "eth1/63/1",
          "controllingPort": 139,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 10,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core13",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip63",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "147": {
        "mapping": {
          "id": 147,
          "name": "eth1/57/1",
          "controllingPort": 147,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 4,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core14",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip57",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "155": {
        "mapping": {
          "id": 155,
          "name": "eth1/60/1",
          "controllingPort": 155,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 1,
          "attachedCorePortIndex": 7,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core15",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip60",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "163": {
        "mapping": {
          "id": 163,
          "name": "eth1/5/1",
          "controllingPort": 163,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 2,
          "attachedCorePortIndex": 8,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core16",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "171": {
        "mapping": {
          "id": 171,
          "name": "eth1/8/1",
          "controllingPort": 171,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 2,
          "attachedCorePortIndex": 11,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core17",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "179": {
        "mapping": {
          "id": 179,
          "name": "eth1/1/1",
          "controllingPort": 179,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 2,
          "attachedCorePortIndex": 4,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core18",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "187": {
        "mapping": {
          "id": 187,
          "name": "eth1/2/1",
          "controllingPort": 187,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 2,
          "attachedCorePortIndex": 5,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core19",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "195": {
        "mapping": {
          "id": 195,
          "name": "eth1/3/1",
          "controllingPort": 195,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 2,
          "attachedCorePortIndex": 6,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core20",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "203": {
        "mapping": {
          "id": 203,
          "name": "eth1/4/1",
          "controllingPort": 203,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 2,
          "attachedCorePortIndex": 7,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core21",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "211": {
        "mapping": {
          "id": 211,
          "name": "eth1/6/1",
          "controllingPort": 211,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 2,
          "attachedCorePortIndex": 9,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core22",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "219": {
        "mapping": {
          "id": 219,
          "name": "eth1/7/1",
          "controllingPort": 219,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 2,
          "attachedCorePortIndex": 10,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core23",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "227": {
        "mapping": {
          "id": 227,
          "name": "eth1/14/1",
          "controllingPort": 227,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 3,
          "attachedCorePortIndex": 9,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core24",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "235": {
        "mapping": {
          "id": 235,
          "name": "eth1/16/1",
          "controllingPort": 235,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 3,
          "attachedCorePortIndex": 11,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core25",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "243": {
        "mapping": {
          "id": 243,
          "name": "eth1/15/1",
          "controllingPort": 243,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 3,
          "attachedCorePortIndex": 10,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core26",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "251": {
        "mapping": {
          "id": 251,
          "name": "eth1/9/1",
          "controllingPort": 251,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 3,
          "attachedCorePortIndex": 4,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core27",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "259": {
        "mapping": {
          "id": 259,
          "name": "eth1/10/1",
          "controllingPort": 259,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 3,
          "attachedCorePortIndex": 5,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core28",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "267": {
        "mapping": {
          "id": 267,
          "name": "eth1/12/1",
          "controllingPort": 267,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 3,
          "attachedCorePortIndex": 7,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core29",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "275": {
        "mapping": {
          "id": 275,
          "name": "eth1/13/1",
          "controllingPort": 275,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 3,
          "attachedCorePortIndex": 8,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core30",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "283": {
        "mapping": {
          "id": 283,
          "name": "eth1/11/1",
          "controllingPort": 283,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 3,
          "attachedCorePortIndex": 6,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core31",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "291": {
        "mapping": {
          "id": 291,
          "name": "eth1/20/1",
          "controllingPort": 291,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 4,
          "attachedCorePortIndex": 8,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core32",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "299": {
        "mapping": {
          "id": 299,
          "name": "eth1/18/1",
          "controllingPort": 299,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 4,
          "attachedCorePortIndex": 6,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core33",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "307": {
        "mapping": {
          "id": 307,
          "name": "eth1/17/1",
          "controllingPort": 307,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 4,
          "attachedCorePortIndex": 5,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core34",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "315": {
        "mapping": {
          "id": 315,
          "name": "eth1/23/1",
          "controllingPort": 315,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 4,
          "attachedCorePortIndex": 11,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core35",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "323": {
        "mapping": {
          "id": 323,
          "name": "eth1/24/1",
          "controllingPort": 323,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 4,
          "attachedCorePortIndex": 12,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core36",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "331": {
        "mapping": {
          "id": 331,
          "name": "eth1/22/1",
          "controllingPort": 331,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 4,
          "attachedCorePortIndex": 10,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core37",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "339": {
        "mapping": {
          "id": 339,
          "name": "eth1/19/1",
          "controllingPort": 339,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 4,
          "attachedCorePortIndex": 7,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core38",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "347": {
        "mapping": {
          "id": 347,
          "name": "eth1/21/1",
          "controllingPort": 347,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 4,
          "attachedCorePortIndex": 9,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core39",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "355": {
        "mapping": {
          "id": 355,
          "name": "eth1/27/1",
          "controllingPort": 355,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 5,
          "attachedCorePortIndex": 6,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core40",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "363": {
        "mapping": {
          "id": 363,
          "name": "eth1/26/1",
          "controllingPort": 363,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 5,
          "attachedCorePortIndex": 5,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core41",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "371": {
        "mapping": {
          "id": 371,
          "name": "eth1/31/1",
          "controllingPort": 371,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 5,
          "attachedCorePortIndex": 10,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core42",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "379": {
        "mapping": {
          "id": 379,
          "name": "eth1/30/1",
          "controllingPort": 379,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 5,
          "attachedCorePortIndex": 9,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core43",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "387": {
        "mapping": {
          "id": 387,
          "name": "eth1/29/1",
          "controllingPort": 387,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 5,
          "attachedCorePortIndex": 8,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core44",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "395": {
        "mapping": {
          "id": 395,
          "name": "eth1/32/1",
          "controllingPort": 395,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 5,
          "attachedCorePortIndex": 11,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core45",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "403": {
        "mapping": {
          "id": 403,
          "name": "eth1/28/1",
          "controllingPort": 403,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 5,
          "attachedCorePortIndex": 7,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core46",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "411": {
        "mapping": {
          "id": 411,
          "name": "eth1/25/1",
          "controllingPort": 411,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 5,
          "attachedCorePortIndex": 4,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core47",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "419": {
        "mapping": {
          "id": 419,
          "name": "eth1/40/1",
          "controllingPort": 419,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 6,
          "attachedCorePortIndex": 11,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core48",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "427": {
        "mapping": {
          "id": 427,
          "name": "eth1/37/1",
          "controllingPort": 427,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 6,
          "attachedCorePortIndex": 8,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core49",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "435": {
        "mapping": {
          "id": 435,
          "name": "eth1/36/1",
          "controllingPort": 435,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 6,
          "attachedCorePortIndex": 7,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core50",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "443": {
        "mapping": {
          "id": 443,
          "name": "eth1/33/1",
          "controllingPort": 443,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 6,
          "attachedCorePortIndex": 4,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core51",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "451": {
        "mapping": {
          "id": 451,
          "name": "eth1/34/1",
          "controllingPort": 451,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 6,
          "attachedCorePortIndex": 5,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core52",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "459": {
        "mapping": {
          "id": 459,
          "name": "eth1/35/1",
          "controllingPort": 459,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 6,
          "attachedCorePortIndex": 6,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core53",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "467": {
        "mapping": {
          "id": 467,
          "name": "eth1/39/1",
          "controllingPort": 467,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 6,
          "attachedCorePortIndex": 10,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core54",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "475": {
        "mapping": {
          "id": 475,
          "name": "eth1/38/1",
          "controllingPort": 475,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 6,
          "attachedCorePortIndex": 9,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core55",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "483": {
        "mapping": {
          "id": 483,
          "name": "eth1/47/1",
          "controllingPort": 483,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 7,
          "attachedCorePortIndex": 10,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core56",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip47",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "491": {
        "mapping": {
          "id": 491,
          "name": "eth1/45/1",
          "controllingPort": 491,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 7,
          "attachedCorePortIndex": 8,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core57",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "499": {
        "mapping": {
          "id": 499,
          "name": "eth1/46/1",
          "controllingPort": 499,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 7,
          "attachedCorePortIndex": 9,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core58",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "507": {
        "mapping": {
          "id": 507,
          "name": "eth1/44/1",
          "controllingPort": 507,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 7,
          "attachedCorePortIndex": 7,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core59",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip44",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "515": {
        "mapping": {
          "id": 515,
          "name": "eth1/43/1",
          "controllingPort": 515,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 7,
          "attachedCorePortIndex": 6,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core60",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "523": {
        "mapping": {
          "id": 523,
          "name": "eth1/41/1",
          "controllingPort": 523,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 7,
          "attachedCorePortIndex": 4,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core61",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip41",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "531": {
        "mapping": {
          "id": 531,
          "name": "eth1/48/1",
          "controllingPort": 531,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 7,
          "attachedCorePortIndex": 11,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core62",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip48",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "539": {
        "mapping": {
          "id": 539,
          "name": "eth1/42/1",
          "controllingPort": 539,
          "pins": [
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "attachedCoreId": 7,
          "attachedCorePortIndex": 5,
          "virtualDeviceId": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "50": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "39": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-Q4D_NIF-slot1/chip1/core63",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    }
  },
  "chips": [
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core0",
      "type": 1,
      "physicalID": 0
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core1",
      "type": 1,
      "physicalID": 1
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core2",
      "type": 1,
      "physicalID": 2
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core3",
      "type": 1,
      "physicalID": 3
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core4",
      "type": 1,
      "physicalID": 4
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core5",
      "type": 1,
      "physicalID": 5
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core6",
      "type": 1,
      "physicalID": 6
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core7",
      "type": 1,
      "physicalID": 7
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core8",
      "type": 1,
      "physicalID": 8
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core9",
      "type": 1,
      "physicalID": 9
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core10",
      "type": 1,
      "physicalID": 10
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core11",
      "type": 1,
      "physicalID": 11
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core12",
      "type": 1,
      "physicalID": 12
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core13",
      "type": 1,
      "physicalID": 13
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core14",
      "type": 1,
      "physicalID": 14
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core15",
      "type": 1,
      "physicalID": 15
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core16",
      "type": 1,
      "physicalID": 16
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core17",
      "type": 1,
      "physicalID": 17
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core18",
      "type": 1,
      "physicalID": 18
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core19",
      "type": 1,
      "physicalID": 19
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core20",
      "type": 1,
      "physicalID": 20
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core21",
      "type": 1,
      "physicalID": 21
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core22",
      "type": 1,
      "physicalID": 22
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core23",
      "type": 1,
      "physicalID": 23
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core24",
      "type": 1,
      "physicalID": 24
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core25",
      "type": 1,
      "physicalID": 25
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core26",
      "type": 1,
      "physicalID": 26
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core27",
      "type": 1,
      "physicalID": 27
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core28",
      "type": 1,
      "physicalID": 28
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core29",
      "type": 1,
      "physicalID": 29
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core30",
      "type": 1,
      "physicalID": 30
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core31",
      "type": 1,
      "physicalID": 31
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core32",
      "type": 1,
      "physicalID": 32
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core33",
      "type": 1,
      "physicalID": 33
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core34",
      "type": 1,
      "physicalID": 34
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core35",
      "type": 1,
      "physicalID": 35
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core36",
      "type": 1,
      "physicalID": 36
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core37",
      "type": 1,
      "physicalID": 37
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core38",
      "type": 1,
      "physicalID": 38
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core39",
      "type": 1,
      "physicalID": 39
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core40",
      "type": 1,
      "physicalID": 40
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core41",
      "type": 1,
      "physicalID": 41
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core42",
      "type": 1,
      "physicalID": 42
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core43",
      "type": 1,
      "physicalID": 43
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core44",
      "type": 1,
      "physicalID": 44
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core45",
      "type": 1,
      "physicalID": 45
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core46",
      "type": 1,
      "physicalID": 46
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core47",
      "type": 1,
      "physicalID": 47
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core48",
      "type": 1,
      "physicalID": 48
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core49",
      "type": 1,
      "physicalID": 49
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core50",
      "type": 1,
      "physicalID": 50
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core51",
      "type": 1,
      "physicalID": 51
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core52",
      "type": 1,
      "physicalID": 52
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core53",
      "type": 1,
      "physicalID": 53
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core54",
      "type": 1,
      "physicalID": 54
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core55",
      "type": 1,
      "physicalID": 55
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core56",
      "type": 1,
      "physicalID": 56
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core57",
      "type": 1,
      "physicalID": 57
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core58",
      "type": 1,
      "physicalID": 58
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core59",
      "type": 1,
      "physicalID": 59
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core60",
      "type": 1,
      "physicalID": 60
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core61",
      "type": 1,
      "physicalID": 61
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core62",
      "type": 1,
      "physicalID": 62
    },
    {
      "name": "NPU-Q4D_NIF-slot1/chip1/core63",
      "type": 1,
      "physicalID": 63
    },
    {
      "name": "NPU-J3_RCY-slot1/chip1/core529",
      "type": 1,
      "physicalID": 529
    },
    {
      "name": "NPU-J3_RCY-slot1/chip1/core530",
      "type": 1,
      "physicalID": 530
    },
    {
      "name": "NPU-J3_RCY-slot1/chip1/core531",
      "type": 1,
      "physicalID": 531
    },
    {
      "name": "NPU-J3_RCY-slot1/chip1/core532",
      "type": 1,
      "physicalID": 532
    },
    {
      "name": "NPU-J3_RCY-slot1/chip1/core533",
      "type": 1,
      "physicalID": 533
    },
    {
      "name": "NPU-J3_RCY-slot1/chip1/core534",
      "type": 1,
      "physicalID": 534
    },
    {
      "name": "NPU-J3_RCY-slot1/chip1/core535",
      "type": 1,
      "physicalID": 535
    },
    {
      "name": "NPU-J3_RCY-slot1/chip1/core536",
      "type": 1,
      "physicalID": 536
    },
    {
      "name": "NPU-J3_RCY-slot1/chip1/core537",
      "type": 1,
      "physicalID": 537
    },
    {
      "name": "NPU-J3_RCY-slot1/chip1/core538",
      "type": 1,
      "physicalID": 538
    },
    {
      "name": "NPU-J3_RCY-slot1/chip1/core539",
      "type": 1,
      "physicalID": 539
    },
    {
      "name": "NPU-J3_RCY-slot1/chip1/core540",
      "type": 1,
      "physicalID": 540
    },
    {
      "name": "NPU-J3_RCY-slot1/chip1/core541",
      "type": 1,
      "physicalID": 541
    },
    {
      "name": "NPU-J3_RCY-slot1/chip1/core542",
      "type": 1,
      "physicalID": 542
    },
    {
      "name": "NPU-J3_RCY-slot1/chip1/core543",
      "type": 1,
      "physicalID": 543
    },
    {
      "name": "NPU-J3_RCY-slot1/chip1/core544",
      "type": 1,
      "physicalID": 544
    },
    {
      "name": "NPU-J3_EVT-slot1/chip1/core1033",
      "type": 1,
      "physicalID": 1033
    },
    {
      "name": "NPU-J3_EVT-slot1/chip1/core1034",
      "type": 1,
      "physicalID": 1034
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip1",
      "type": 3,
      "physicalID": 0
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip2",
      "type": 3,
      "physicalID": 1
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip3",
      "type": 3,
      "physicalID": 2
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip4",
      "type": 3,
      "physicalID": 3
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip5",
      "type": 3,
      "physicalID": 4
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip6",
      "type": 3,
      "physicalID": 5
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip7",
      "type": 3,
      "physicalID": 6
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip8",
      "type": 3,
      "physicalID": 7
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip9",
      "type": 3,
      "physicalID": 8
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip10",
      "type": 3,
      "physicalID": 9
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip11",
      "type": 3,
      "physicalID": 10
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip12",
      "type": 3,
      "physicalID": 11
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip13",
      "type": 3,
      "physicalID": 12
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip14",
      "type": 3,
      "physicalID": 13
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip15",
      "type": 3,
      "physicalID": 14
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip16",
      "type": 3,
      "physicalID": 15
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip17",
      "type": 3,
      "physicalID": 16
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip18",
      "type": 3,
      "physicalID": 17
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip19",
      "type": 3,
      "physicalID": 18
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip20",
      "type": 3,
      "physicalID": 19
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip21",
      "type": 3,
      "physicalID": 20
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip22",
      "type": 3,
      "physicalID": 21
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip23",
      "type": 3,
      "physicalID": 22
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip24",
      "type": 3,
      "physicalID": 23
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip25",
      "type": 3,
      "physicalID": 24
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip26",
      "type": 3,
      "physicalID": 25
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip27",
      "type": 3,
      "physicalID": 26
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip28",
      "type": 3,
      "physicalID": 27
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip29",
      "type": 3,
      "physicalID": 28
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip30",
      "type": 3,
      "physicalID": 29
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip31",
      "type": 3,
      "physicalID": 30
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip32",
      "type": 3,
      "physicalID": 31
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip33",
      "type": 3,
      "physicalID": 32
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip34",
      "type": 3,
      "physicalID": 33
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip35",
      "type": 3,
      "physicalID": 34
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip36",
      "type": 3,
      "physicalID": 35
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip37",
      "type": 3,
      "physicalID": 36
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip38",
      "type": 3,
      "physicalID": 37
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip39",
      "type": 3,
      "physicalID": 38
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip40",
      "type": 3,
      "physicalID": 39
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip41",
      "type": 3,
      "physicalID": 40
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip42",
      "type": 3,
      "physicalID": 41
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip43",
      "type": 3,
      "physicalID": 42
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip44",
      "type": 3,
      "physicalID": 43
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip45",
      "type": 3,
      "physicalID": 44
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip46",
      "type": 3,
      "physicalID": 45
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip47",
      "type": 3,
      "physicalID": 46
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip48",
      "type": 3,
      "physicalID": 47
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip49",
      "type": 3,
      "physicalID": 48
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip50",
      "type": 3,
      "physicalID": 49
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip51",
      "type": 3,
      "physicalID": 50
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip52",
      "type": 3,
      "physicalID": 51
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip53",
      "type": 3,
      "physicalID": 52
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip54",
      "type": 3,
      "physicalID": 53
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip55",
      "type": 3,
      "physicalID": 54
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip56",
      "type": 3,
      "physicalID": 55
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip57",
      "type": 3,
      "physicalID": 56
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip58",
      "type": 3,
      "physicalID": 57
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip59",
      "type": 3,
      "physicalID": 58
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip60",
      "type": 3,
      "physicalID": 59
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip61",
      "type": 3,
      "physicalID": 60
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip62",
      "type": 3,
      "physicalID": 61
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip63",
      "type": 3,
      "physicalID": 62
    },
    {
      "name": "TRANSCEIVER-OSFP-slot1/chip64",
      "type": 3,
      "physicalID": 63
    }
  ],
  "platformSupportedProfiles": [
    {
      "factor": {
        "profileID": 49
      },
      "profile": {
        "speed": 100000,
        "iphy": {
          "numLanes": 1,
          "modulation": 2,
          "fec": 1,
          "medium": 1,
          "interfaceType": 10
        }
      }
    },
    {
      "factor": {
        "profileID": 50
      },
      "profile": {
        "speed": 800000,
        "iphy": {
          "numLanes": 8,
          "modulation": 2,
          "fec": 11,
          "medium": 1,
          "interfaceType": 13
        }
      }
    },
    {
      "factor": {
        "profileID": 39
      },
      "profile": {
        "speed": 800000,
        "iphy": {
          "numLanes": 8,
          "modulation": 2,
          "fec": 11,
          "medium": 2,
          "interfaceType": 23
        }
      }
    }
  ]
}
)";
} // namespace

namespace facebook::fboss {
Blackwolf800banwPlatformMapping::Blackwolf800banwPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

Blackwolf800banwPlatformMapping::Blackwolf800banwPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

std::map<uint32_t, std::pair<uint32_t, uint32_t>>
Blackwolf800banwPlatformMapping::getCpuPortsCoreAndPortIdx() const {
  static const std::map<uint32_t, std::pair<uint32_t, uint32_t>>
      kSingleStageCpuPortsCoreAndPortIdx = {
          // {CPU System Port, {Core ID, Port ID/PP_DSP}}
          {0, {0, 0}},
          {1, {0, 1}},
          {2, {0, 2}},
          {3, {1, 0}},
          {4, {1, 1}},
          {5, {2, 0}},
          {6, {2, 1}},
          {7, {3, 0}},
          {8, {3, 1}},
          {9, {4, 0}},
          {10, {4, 1}},
          {11, {5, 0}},
          {12, {5, 1}},
          {13, {6, 0}},
          {14, {6, 1}},
          {15, {7, 0}},
          {16, {7, 1}},
      };
  return kSingleStageCpuPortsCoreAndPortIdx;
}

} // namespace facebook::fboss
