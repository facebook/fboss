/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/janga800bic/Janga800bicPlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{
  "ports": {
    "1": {
        "mapping": {
          "id": 1,
          "name": "eth1/1/1",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip1/core0",
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
                "chip": "NPU-J3_NIF-slot1/chip1/core0",
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
                "chip": "NPU-J3_NIF-slot1/chip1/core0",
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
                "chip": "NPU-J3_NIF-slot1/chip1/core0",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip1",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core0",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core0",
                      "lane": 3
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
                  }
                ]
              }
          }
        }
    },
    "2": {
        "mapping": {
          "id": 2,
          "name": "eth1/1/5",
          "controllingPort": 2,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip1/core0",
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
                "chip": "NPU-J3_NIF-slot1/chip1/core0",
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
                "chip": "NPU-J3_NIF-slot1/chip1/core0",
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
                "chip": "NPU-J3_NIF-slot1/chip1/core0",
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
          "portType": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core0",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core0",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core0",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core0",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "3": {
        "mapping": {
          "id": 3,
          "name": "eth1/2/1",
          "controllingPort": 3,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip1/core13",
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
                "chip": "NPU-J3_NIF-slot1/chip1/core13",
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
                "chip": "NPU-J3_NIF-slot1/chip1/core13",
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
                "chip": "NPU-J3_NIF-slot1/chip1/core13",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip2",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core13",
                      "lane": 3
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
                  }
                ]
              }
          }
        }
    },
    "4": {
        "mapping": {
          "id": 4,
          "name": "eth1/2/5",
          "controllingPort": 4,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip1/core13",
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
                "chip": "NPU-J3_NIF-slot1/chip1/core13",
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
                "chip": "NPU-J3_NIF-slot1/chip1/core13",
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
                "chip": "NPU-J3_NIF-slot1/chip1/core13",
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
          "portType": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core13",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "5": {
        "mapping": {
          "id": 5,
          "name": "fab1/3/1",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core33",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core33",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "6": {
        "mapping": {
          "id": 6,
          "name": "fab1/3/2",
          "controllingPort": 6,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core33",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core33",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "7": {
        "mapping": {
          "id": 7,
          "name": "fab1/3/3",
          "controllingPort": 7,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core33",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core33",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "8": {
        "mapping": {
          "id": 8,
          "name": "fab1/3/4",
          "controllingPort": 8,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core33",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core33",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "9": {
        "mapping": {
          "id": 9,
          "name": "fab1/3/5",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core33",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core33",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "10": {
        "mapping": {
          "id": 10,
          "name": "fab1/3/6",
          "controllingPort": 10,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core33",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core33",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "11": {
        "mapping": {
          "id": 11,
          "name": "fab1/3/7",
          "controllingPort": 11,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core33",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core33",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip3",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "12": {
        "mapping": {
          "id": 12,
          "name": "fab1/3/8",
          "controllingPort": 12,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core33",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core33",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "13": {
        "mapping": {
          "id": 13,
          "name": "fab1/4/1",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core0",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core0",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "14": {
        "mapping": {
          "id": 14,
          "name": "fab1/4/2",
          "controllingPort": 14,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core0",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core0",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "15": {
        "mapping": {
          "id": 15,
          "name": "fab1/4/3",
          "controllingPort": 15,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core0",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core0",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "16": {
        "mapping": {
          "id": 16,
          "name": "fab1/4/4",
          "controllingPort": 16,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core0",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "17": {
        "mapping": {
          "id": 17,
          "name": "fab1/4/5",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core0",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core0",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 4
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
          "name": "fab1/4/6",
          "controllingPort": 18,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core0",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core0",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 5
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
          "name": "fab1/4/7",
          "controllingPort": 19,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core0",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core0",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip4",
                      "lane": 6
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
          "name": "fab1/4/8",
          "controllingPort": 20,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core0",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core0",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "21": {
        "mapping": {
          "id": 21,
          "name": "fab1/5/1",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core13",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core13",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
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
          "name": "fab1/5/2",
          "controllingPort": 22,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core13",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core13",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 1
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
          "name": "fab1/5/3",
          "controllingPort": 23,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core13",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core13",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 2
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
          "name": "fab1/5/4",
          "controllingPort": 24,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core13",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 3
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
          "name": "fab1/5/5",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core13",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core13",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 4
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
          "name": "fab1/5/6",
          "controllingPort": 26,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core13",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core13",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 5
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
          "name": "fab1/5/7",
          "controllingPort": 27,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core13",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core13",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip5",
                      "lane": 6
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
          "name": "fab1/5/8",
          "controllingPort": 28,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core13",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core13",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "29": {
        "mapping": {
          "id": 29,
          "name": "fab1/6/1",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core36",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core36",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
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
          "name": "fab1/6/2",
          "controllingPort": 30,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core36",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core36",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 1
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
          "name": "fab1/6/3",
          "controllingPort": 31,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core36",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core36",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 2
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
          "name": "fab1/6/4",
          "controllingPort": 32,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core36",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core36",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 3
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
          "name": "fab1/6/5",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core36",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core36",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 4
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
          "name": "fab1/6/6",
          "controllingPort": 34,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core36",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core36",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 5
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
          "name": "fab1/6/7",
          "controllingPort": 35,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core36",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core36",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip6",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "36": {
        "mapping": {
          "id": 36,
          "name": "fab1/6/8",
          "controllingPort": 36,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core36",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core36",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "37": {
        "mapping": {
          "id": 37,
          "name": "fab1/7/1",
          "controllingPort": 37,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core19",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core19",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "38": {
        "mapping": {
          "id": 38,
          "name": "fab1/7/2",
          "controllingPort": 38,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core19",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core19",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "39": {
        "mapping": {
          "id": 39,
          "name": "fab1/7/3",
          "controllingPort": 39,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core19",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core19",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "40": {
        "mapping": {
          "id": 40,
          "name": "fab1/7/4",
          "controllingPort": 40,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core19",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "41": {
        "mapping": {
          "id": 41,
          "name": "fab1/7/5",
          "controllingPort": 41,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core19",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core19",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "42": {
        "mapping": {
          "id": 42,
          "name": "fab1/7/6",
          "controllingPort": 42,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core19",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core19",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 5
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
          "name": "fab1/7/7",
          "controllingPort": 43,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core19",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core19",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip7",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "44": {
        "mapping": {
          "id": 44,
          "name": "fab1/7/8",
          "controllingPort": 44,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core19",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core19",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "45": {
        "mapping": {
          "id": 45,
          "name": "fab1/8/1",
          "controllingPort": 45,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core20",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core20",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "46": {
        "mapping": {
          "id": 46,
          "name": "fab1/8/2",
          "controllingPort": 46,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core20",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "47": {
        "mapping": {
          "id": 47,
          "name": "fab1/8/3",
          "controllingPort": 47,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core20",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core20",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "48": {
        "mapping": {
          "id": 48,
          "name": "fab1/8/4",
          "controllingPort": 48,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core20",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "49": {
        "mapping": {
          "id": 49,
          "name": "fab1/8/5",
          "controllingPort": 49,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core20",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core20",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "50": {
        "mapping": {
          "id": 50,
          "name": "fab1/8/6",
          "controllingPort": 50,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core20",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core20",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 5
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
          "name": "fab1/8/7",
          "controllingPort": 51,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core20",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core20",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip8",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "52": {
        "mapping": {
          "id": 52,
          "name": "fab1/8/8",
          "controllingPort": 52,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core20",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core20",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "53": {
        "mapping": {
          "id": 53,
          "name": "fab1/9/1",
          "controllingPort": 53,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core21",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core21",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "54": {
        "mapping": {
          "id": 54,
          "name": "fab1/9/2",
          "controllingPort": 54,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core21",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "55": {
        "mapping": {
          "id": 55,
          "name": "fab1/9/3",
          "controllingPort": 55,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core21",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core21",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "56": {
        "mapping": {
          "id": 56,
          "name": "fab1/9/4",
          "controllingPort": 56,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core21",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "57": {
        "mapping": {
          "id": 57,
          "name": "fab1/9/5",
          "controllingPort": 57,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core21",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core21",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "58": {
        "mapping": {
          "id": 58,
          "name": "fab1/9/6",
          "controllingPort": 58,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core21",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core21",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 5
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
          "name": "fab1/9/7",
          "controllingPort": 59,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core21",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core21",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip9",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "60": {
        "mapping": {
          "id": 60,
          "name": "fab1/9/8",
          "controllingPort": 60,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core21",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core21",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "61": {
        "mapping": {
          "id": 61,
          "name": "fab1/10/1",
          "controllingPort": 61,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core22",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core22",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "62": {
        "mapping": {
          "id": 62,
          "name": "fab1/10/2",
          "controllingPort": 62,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core22",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "63": {
        "mapping": {
          "id": 63,
          "name": "fab1/10/3",
          "controllingPort": 63,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core22",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core22",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "64": {
        "mapping": {
          "id": 64,
          "name": "fab1/10/4",
          "controllingPort": 64,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core22",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "65": {
        "mapping": {
          "id": 65,
          "name": "fab1/10/5",
          "controllingPort": 65,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core22",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core22",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "66": {
        "mapping": {
          "id": 66,
          "name": "fab1/10/6",
          "controllingPort": 66,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core22",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core22",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 5
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
          "name": "fab1/10/7",
          "controllingPort": 67,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core22",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core22",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip10",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "68": {
        "mapping": {
          "id": 68,
          "name": "fab1/10/8",
          "controllingPort": 68,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core22",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core22",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "69": {
        "mapping": {
          "id": 69,
          "name": "fab1/11/1",
          "controllingPort": 69,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core23",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core23",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "70": {
        "mapping": {
          "id": 70,
          "name": "fab1/11/2",
          "controllingPort": 70,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core23",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "71": {
        "mapping": {
          "id": 71,
          "name": "fab1/11/3",
          "controllingPort": 71,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core23",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core23",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "72": {
        "mapping": {
          "id": 72,
          "name": "fab1/11/4",
          "controllingPort": 72,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core23",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "73": {
        "mapping": {
          "id": 73,
          "name": "fab1/11/5",
          "controllingPort": 73,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core23",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core23",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "74": {
        "mapping": {
          "id": 74,
          "name": "fab1/11/6",
          "controllingPort": 74,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core23",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core23",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 5
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
          "name": "fab1/11/7",
          "controllingPort": 75,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core23",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core23",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip11",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "76": {
        "mapping": {
          "id": 76,
          "name": "fab1/11/8",
          "controllingPort": 76,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core23",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core23",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "77": {
        "mapping": {
          "id": 77,
          "name": "fab1/12/1",
          "controllingPort": 77,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core24",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core24",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "78": {
        "mapping": {
          "id": 78,
          "name": "fab1/12/2",
          "controllingPort": 78,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core24",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core24",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "79": {
        "mapping": {
          "id": 79,
          "name": "fab1/12/3",
          "controllingPort": 79,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core24",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core24",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "80": {
        "mapping": {
          "id": 80,
          "name": "fab1/12/4",
          "controllingPort": 80,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core24",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "81": {
        "mapping": {
          "id": 81,
          "name": "fab1/12/5",
          "controllingPort": 81,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core24",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core24",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "82": {
        "mapping": {
          "id": 82,
          "name": "fab1/12/6",
          "controllingPort": 82,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core24",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core24",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 5
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
          "name": "fab1/12/7",
          "controllingPort": 83,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core24",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core24",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip12",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "84": {
        "mapping": {
          "id": 84,
          "name": "fab1/12/8",
          "controllingPort": 84,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core24",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core24",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "85": {
        "mapping": {
          "id": 85,
          "name": "fab1/13/1",
          "controllingPort": 85,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core25",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core25",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "86": {
        "mapping": {
          "id": 86,
          "name": "fab1/13/2",
          "controllingPort": 86,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core25",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core25",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "87": {
        "mapping": {
          "id": 87,
          "name": "fab1/13/3",
          "controllingPort": 87,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core25",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core25",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "88": {
        "mapping": {
          "id": 88,
          "name": "fab1/13/4",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core25",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "89": {
        "mapping": {
          "id": 89,
          "name": "fab1/13/5",
          "controllingPort": 89,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core25",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core25",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "90": {
        "mapping": {
          "id": 90,
          "name": "fab1/13/6",
          "controllingPort": 90,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core25",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core25",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 5
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
          "name": "fab1/13/7",
          "controllingPort": 91,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core25",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core25",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip13",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "92": {
        "mapping": {
          "id": 92,
          "name": "fab1/13/8",
          "controllingPort": 92,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core25",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core25",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "93": {
        "mapping": {
          "id": 93,
          "name": "fab1/14/1",
          "controllingPort": 93,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core26",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core26",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "94": {
        "mapping": {
          "id": 94,
          "name": "fab1/14/2",
          "controllingPort": 94,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core26",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core26",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "95": {
        "mapping": {
          "id": 95,
          "name": "fab1/14/3",
          "controllingPort": 95,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core26",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core26",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "96": {
        "mapping": {
          "id": 96,
          "name": "fab1/14/4",
          "controllingPort": 96,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core26",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "97": {
        "mapping": {
          "id": 97,
          "name": "fab1/14/5",
          "controllingPort": 97,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core26",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core26",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "98": {
        "mapping": {
          "id": 98,
          "name": "fab1/14/6",
          "controllingPort": 98,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core26",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core26",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 5
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
          "name": "fab1/14/7",
          "controllingPort": 99,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core26",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core26",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip14",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "100": {
        "mapping": {
          "id": 100,
          "name": "fab1/14/8",
          "controllingPort": 100,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core26",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core26",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "101": {
        "mapping": {
          "id": 101,
          "name": "fab1/15/1",
          "controllingPort": 101,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core27",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core27",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "102": {
        "mapping": {
          "id": 102,
          "name": "fab1/15/2",
          "controllingPort": 102,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core27",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core27",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "103": {
        "mapping": {
          "id": 103,
          "name": "fab1/15/3",
          "controllingPort": 103,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core27",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core27",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "104": {
        "mapping": {
          "id": 104,
          "name": "fab1/15/4",
          "controllingPort": 104,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core27",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "105": {
        "mapping": {
          "id": 105,
          "name": "fab1/15/5",
          "controllingPort": 105,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core27",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core27",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "106": {
        "mapping": {
          "id": 106,
          "name": "fab1/15/6",
          "controllingPort": 106,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core27",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core27",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 5
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
          "name": "fab1/15/7",
          "controllingPort": 107,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core27",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core27",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip15",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "108": {
        "mapping": {
          "id": 108,
          "name": "fab1/15/8",
          "controllingPort": 108,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core27",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core27",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "109": {
        "mapping": {
          "id": 109,
          "name": "fab1/16/1",
          "controllingPort": 109,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core28",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core28",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "110": {
        "mapping": {
          "id": 110,
          "name": "fab1/16/2",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core28",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core28",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "111": {
        "mapping": {
          "id": 111,
          "name": "fab1/16/3",
          "controllingPort": 111,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core28",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core28",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "112": {
        "mapping": {
          "id": 112,
          "name": "fab1/16/4",
          "controllingPort": 112,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core28",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "113": {
        "mapping": {
          "id": 113,
          "name": "fab1/16/5",
          "controllingPort": 113,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core28",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core28",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "114": {
        "mapping": {
          "id": 114,
          "name": "fab1/16/6",
          "controllingPort": 114,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core28",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core28",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 5
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
          "name": "fab1/16/7",
          "controllingPort": 115,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core28",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core28",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip16",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "116": {
        "mapping": {
          "id": 116,
          "name": "fab1/16/8",
          "controllingPort": 116,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core28",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core28",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "117": {
        "mapping": {
          "id": 117,
          "name": "fab1/17/1",
          "controllingPort": 117,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core29",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core29",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "118": {
        "mapping": {
          "id": 118,
          "name": "fab1/17/2",
          "controllingPort": 118,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core29",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core29",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "119": {
        "mapping": {
          "id": 119,
          "name": "fab1/17/3",
          "controllingPort": 119,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core29",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core29",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "120": {
        "mapping": {
          "id": 120,
          "name": "fab1/17/4",
          "controllingPort": 120,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core29",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "121": {
        "mapping": {
          "id": 121,
          "name": "fab1/17/5",
          "controllingPort": 121,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core29",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core29",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "122": {
        "mapping": {
          "id": 122,
          "name": "fab1/17/6",
          "controllingPort": 122,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core29",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core29",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 5
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
          "name": "fab1/17/7",
          "controllingPort": 123,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core29",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core29",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip17",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "124": {
        "mapping": {
          "id": 124,
          "name": "fab1/17/8",
          "controllingPort": 124,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core29",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core29",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "125": {
        "mapping": {
          "id": 125,
          "name": "fab1/18/1",
          "controllingPort": 125,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core30",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core30",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "126": {
        "mapping": {
          "id": 126,
          "name": "fab1/18/2",
          "controllingPort": 126,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core30",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core30",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "127": {
        "mapping": {
          "id": 127,
          "name": "fab1/18/3",
          "controllingPort": 127,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core30",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core30",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "128": {
        "mapping": {
          "id": 128,
          "name": "fab1/18/4",
          "controllingPort": 128,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core30",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "129": {
        "mapping": {
          "id": 129,
          "name": "fab1/18/5",
          "controllingPort": 129,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core30",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core30",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "130": {
        "mapping": {
          "id": 130,
          "name": "fab1/18/6",
          "controllingPort": 130,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core30",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core30",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 5
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
          "name": "fab1/18/7",
          "controllingPort": 131,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core30",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core30",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip18",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "132": {
        "mapping": {
          "id": 132,
          "name": "fab1/18/8",
          "controllingPort": 132,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core30",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core30",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "133": {
        "mapping": {
          "id": 133,
          "name": "fab1/19/1",
          "controllingPort": 133,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core31",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core31",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "134": {
        "mapping": {
          "id": 134,
          "name": "fab1/19/2",
          "controllingPort": 134,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core31",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core31",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "135": {
        "mapping": {
          "id": 135,
          "name": "fab1/19/3",
          "controllingPort": 135,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core31",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core31",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "136": {
        "mapping": {
          "id": 136,
          "name": "fab1/19/4",
          "controllingPort": 136,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core31",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "137": {
        "mapping": {
          "id": 137,
          "name": "fab1/19/5",
          "controllingPort": 137,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core31",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core31",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "138": {
        "mapping": {
          "id": 138,
          "name": "fab1/19/6",
          "controllingPort": 138,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core31",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core31",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 5
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
          "name": "fab1/19/7",
          "controllingPort": 139,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core31",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core31",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip19",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "140": {
        "mapping": {
          "id": 140,
          "name": "fab1/19/8",
          "controllingPort": 140,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core31",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core31",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "141": {
        "mapping": {
          "id": 141,
          "name": "fab1/20/1",
          "controllingPort": 141,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core32",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core32",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "142": {
        "mapping": {
          "id": 142,
          "name": "fab1/20/2",
          "controllingPort": 142,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core32",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core32",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "143": {
        "mapping": {
          "id": 143,
          "name": "fab1/20/3",
          "controllingPort": 143,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core32",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core32",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "144": {
        "mapping": {
          "id": 144,
          "name": "fab1/20/4",
          "controllingPort": 144,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core32",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core32",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "145": {
        "mapping": {
          "id": 145,
          "name": "fab1/20/5",
          "controllingPort": 145,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core32",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core32",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "146": {
        "mapping": {
          "id": 146,
          "name": "fab1/20/6",
          "controllingPort": 146,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core32",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core32",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 5
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
          "name": "fab1/20/7",
          "controllingPort": 147,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core32",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core32",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip20",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "148": {
        "mapping": {
          "id": 148,
          "name": "fab1/20/8",
          "controllingPort": 148,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core32",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core32",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "149": {
        "mapping": {
          "id": 149,
          "name": "fab1/21/1",
          "controllingPort": 149,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core33",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core33",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "150": {
        "mapping": {
          "id": 150,
          "name": "fab1/21/2",
          "controllingPort": 150,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core33",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core33",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "151": {
        "mapping": {
          "id": 151,
          "name": "fab1/21/3",
          "controllingPort": 151,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core33",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core33",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "152": {
        "mapping": {
          "id": 152,
          "name": "fab1/21/4",
          "controllingPort": 152,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core33",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core33",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "153": {
        "mapping": {
          "id": 153,
          "name": "fab1/21/5",
          "controllingPort": 153,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core33",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core33",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "154": {
        "mapping": {
          "id": 154,
          "name": "fab1/21/6",
          "controllingPort": 154,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core33",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core33",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 5
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
          "name": "fab1/21/7",
          "controllingPort": 155,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core33",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core33",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip21",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "156": {
        "mapping": {
          "id": 156,
          "name": "fab1/21/8",
          "controllingPort": 156,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core33",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core33",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "157": {
        "mapping": {
          "id": 157,
          "name": "fab1/22/1",
          "controllingPort": 157,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core34",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core34",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "158": {
        "mapping": {
          "id": 158,
          "name": "fab1/22/2",
          "controllingPort": 158,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core34",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core34",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "159": {
        "mapping": {
          "id": 159,
          "name": "fab1/22/3",
          "controllingPort": 159,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core34",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core34",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "160": {
        "mapping": {
          "id": 160,
          "name": "fab1/22/4",
          "controllingPort": 160,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core34",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core34",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "161": {
        "mapping": {
          "id": 161,
          "name": "fab1/22/5",
          "controllingPort": 161,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core34",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core34",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "162": {
        "mapping": {
          "id": 162,
          "name": "fab1/22/6",
          "controllingPort": 162,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core34",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core34",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 5
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
          "name": "fab1/22/7",
          "controllingPort": 163,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core34",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core34",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip22",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "164": {
        "mapping": {
          "id": 164,
          "name": "fab1/22/8",
          "controllingPort": 164,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core34",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core34",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "165": {
        "mapping": {
          "id": 165,
          "name": "fab1/23/1",
          "controllingPort": 165,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core35",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core35",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "166": {
        "mapping": {
          "id": 166,
          "name": "fab1/23/2",
          "controllingPort": 166,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core35",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core35",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "167": {
        "mapping": {
          "id": 167,
          "name": "fab1/23/3",
          "controllingPort": 167,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core35",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core35",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "168": {
        "mapping": {
          "id": 168,
          "name": "fab1/23/4",
          "controllingPort": 168,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core35",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core35",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "169": {
        "mapping": {
          "id": 169,
          "name": "fab1/23/5",
          "controllingPort": 169,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core35",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core35",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "170": {
        "mapping": {
          "id": 170,
          "name": "fab1/23/6",
          "controllingPort": 170,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core35",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core35",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 5
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
          "name": "fab1/23/7",
          "controllingPort": 171,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core35",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core35",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip23",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "172": {
        "mapping": {
          "id": 172,
          "name": "fab1/23/8",
          "controllingPort": 172,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core35",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core35",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "173": {
        "mapping": {
          "id": 173,
          "name": "fab1/24/1",
          "controllingPort": 173,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core36",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core36",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "174": {
        "mapping": {
          "id": 174,
          "name": "fab1/24/2",
          "controllingPort": 174,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core36",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core36",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "175": {
        "mapping": {
          "id": 175,
          "name": "fab1/24/3",
          "controllingPort": 175,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core36",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core36",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "176": {
        "mapping": {
          "id": 176,
          "name": "fab1/24/4",
          "controllingPort": 176,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core36",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core36",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "177": {
        "mapping": {
          "id": 177,
          "name": "fab1/24/5",
          "controllingPort": 177,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core36",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core36",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "178": {
        "mapping": {
          "id": 178,
          "name": "fab1/24/6",
          "controllingPort": 178,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core36",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core36",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 5
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
          "name": "fab1/24/7",
          "controllingPort": 179,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core36",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core36",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip24",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "180": {
        "mapping": {
          "id": 180,
          "name": "fab1/24/8",
          "controllingPort": 180,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core36",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core36",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "181": {
        "mapping": {
          "id": 181,
          "name": "fab1/25/1",
          "controllingPort": 181,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core37",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core37",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "182": {
        "mapping": {
          "id": 182,
          "name": "fab1/25/2",
          "controllingPort": 182,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core37",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core37",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "183": {
        "mapping": {
          "id": 183,
          "name": "fab1/25/3",
          "controllingPort": 183,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core37",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core37",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "184": {
        "mapping": {
          "id": 184,
          "name": "fab1/25/4",
          "controllingPort": 184,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core37",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core37",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "185": {
        "mapping": {
          "id": 185,
          "name": "fab1/25/5",
          "controllingPort": 185,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core37",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core37",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "186": {
        "mapping": {
          "id": 186,
          "name": "fab1/25/6",
          "controllingPort": 186,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core37",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core37",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 5
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
          "name": "fab1/25/7",
          "controllingPort": 187,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core37",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core37",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip25",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "188": {
        "mapping": {
          "id": 188,
          "name": "fab1/25/8",
          "controllingPort": 188,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core37",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core37",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "189": {
        "mapping": {
          "id": 189,
          "name": "fab1/26/1",
          "controllingPort": 189,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core38",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core38",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "190": {
        "mapping": {
          "id": 190,
          "name": "fab1/26/2",
          "controllingPort": 190,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core38",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core38",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "191": {
        "mapping": {
          "id": 191,
          "name": "fab1/26/3",
          "controllingPort": 191,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core38",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core38",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "192": {
        "mapping": {
          "id": 192,
          "name": "fab1/26/4",
          "controllingPort": 192,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core38",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core38",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "193": {
        "mapping": {
          "id": 193,
          "name": "fab1/26/5",
          "controllingPort": 193,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core38",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core38",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "194": {
        "mapping": {
          "id": 194,
          "name": "fab1/26/6",
          "controllingPort": 194,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core38",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core38",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 5
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
          "name": "fab1/26/7",
          "controllingPort": 195,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core38",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core38",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip26",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "196": {
        "mapping": {
          "id": 196,
          "name": "fab1/26/8",
          "controllingPort": 196,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip1/core38",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip1/core38",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "197": {
        "mapping": {
          "id": 197,
          "name": "fab1/27/1",
          "controllingPort": 197,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core19",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core19",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "198": {
        "mapping": {
          "id": 198,
          "name": "fab1/27/2",
          "controllingPort": 198,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core19",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core19",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "199": {
        "mapping": {
          "id": 199,
          "name": "fab1/27/3",
          "controllingPort": 199,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core19",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core19",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "200": {
        "mapping": {
          "id": 200,
          "name": "fab1/27/4",
          "controllingPort": 200,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core19",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "201": {
        "mapping": {
          "id": 201,
          "name": "fab1/27/5",
          "controllingPort": 201,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core19",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core19",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "202": {
        "mapping": {
          "id": 202,
          "name": "fab1/27/6",
          "controllingPort": 202,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core19",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core19",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 5
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
          "name": "fab1/27/7",
          "controllingPort": 203,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core19",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core19",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip27",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "204": {
        "mapping": {
          "id": 204,
          "name": "fab1/27/8",
          "controllingPort": 204,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core19",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core19",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "205": {
        "mapping": {
          "id": 205,
          "name": "fab1/28/1",
          "controllingPort": 205,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core20",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core20",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "206": {
        "mapping": {
          "id": 206,
          "name": "fab1/28/2",
          "controllingPort": 206,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core20",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "207": {
        "mapping": {
          "id": 207,
          "name": "fab1/28/3",
          "controllingPort": 207,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core20",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core20",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "208": {
        "mapping": {
          "id": 208,
          "name": "fab1/28/4",
          "controllingPort": 208,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core20",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "209": {
        "mapping": {
          "id": 209,
          "name": "fab1/28/5",
          "controllingPort": 209,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core20",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core20",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "210": {
        "mapping": {
          "id": 210,
          "name": "fab1/28/6",
          "controllingPort": 210,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core20",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core20",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 5
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
          "name": "fab1/28/7",
          "controllingPort": 211,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core20",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core20",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip28",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "212": {
        "mapping": {
          "id": 212,
          "name": "fab1/28/8",
          "controllingPort": 212,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core20",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core20",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "213": {
        "mapping": {
          "id": 213,
          "name": "fab1/29/1",
          "controllingPort": 213,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core21",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core21",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "214": {
        "mapping": {
          "id": 214,
          "name": "fab1/29/2",
          "controllingPort": 214,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core21",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "215": {
        "mapping": {
          "id": 215,
          "name": "fab1/29/3",
          "controllingPort": 215,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core21",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core21",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "216": {
        "mapping": {
          "id": 216,
          "name": "fab1/29/4",
          "controllingPort": 216,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core21",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "217": {
        "mapping": {
          "id": 217,
          "name": "fab1/29/5",
          "controllingPort": 217,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core21",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core21",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "218": {
        "mapping": {
          "id": 218,
          "name": "fab1/29/6",
          "controllingPort": 218,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core21",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core21",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 5
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
          "name": "fab1/29/7",
          "controllingPort": 219,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core21",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core21",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip29",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "220": {
        "mapping": {
          "id": 220,
          "name": "fab1/29/8",
          "controllingPort": 220,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core21",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core21",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "221": {
        "mapping": {
          "id": 221,
          "name": "fab1/30/1",
          "controllingPort": 221,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core22",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core22",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "222": {
        "mapping": {
          "id": 222,
          "name": "fab1/30/2",
          "controllingPort": 222,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core22",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "223": {
        "mapping": {
          "id": 223,
          "name": "fab1/30/3",
          "controllingPort": 223,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core22",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core22",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "224": {
        "mapping": {
          "id": 224,
          "name": "fab1/30/4",
          "controllingPort": 224,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core22",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "225": {
        "mapping": {
          "id": 225,
          "name": "fab1/30/5",
          "controllingPort": 225,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core22",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core22",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "226": {
        "mapping": {
          "id": 226,
          "name": "fab1/30/6",
          "controllingPort": 226,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core22",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core22",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 5
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
          "name": "fab1/30/7",
          "controllingPort": 227,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core22",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core22",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip30",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "228": {
        "mapping": {
          "id": 228,
          "name": "fab1/30/8",
          "controllingPort": 228,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core22",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core22",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "229": {
        "mapping": {
          "id": 229,
          "name": "fab1/31/1",
          "controllingPort": 229,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core23",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core23",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "230": {
        "mapping": {
          "id": 230,
          "name": "fab1/31/2",
          "controllingPort": 230,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core23",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "231": {
        "mapping": {
          "id": 231,
          "name": "fab1/31/3",
          "controllingPort": 231,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core23",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core23",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "232": {
        "mapping": {
          "id": 232,
          "name": "fab1/31/4",
          "controllingPort": 232,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core23",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "233": {
        "mapping": {
          "id": 233,
          "name": "fab1/31/5",
          "controllingPort": 233,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core23",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core23",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "234": {
        "mapping": {
          "id": 234,
          "name": "fab1/31/6",
          "controllingPort": 234,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core23",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core23",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 5
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
          "name": "fab1/31/7",
          "controllingPort": 235,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core23",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core23",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip31",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "236": {
        "mapping": {
          "id": 236,
          "name": "fab1/31/8",
          "controllingPort": 236,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core23",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core23",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "237": {
        "mapping": {
          "id": 237,
          "name": "fab1/32/1",
          "controllingPort": 237,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core24",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core24",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "238": {
        "mapping": {
          "id": 238,
          "name": "fab1/32/2",
          "controllingPort": 238,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core24",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core24",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "239": {
        "mapping": {
          "id": 239,
          "name": "fab1/32/3",
          "controllingPort": 239,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core24",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core24",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "240": {
        "mapping": {
          "id": 240,
          "name": "fab1/32/4",
          "controllingPort": 240,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core24",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "241": {
        "mapping": {
          "id": 241,
          "name": "fab1/32/5",
          "controllingPort": 241,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core24",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core24",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "242": {
        "mapping": {
          "id": 242,
          "name": "fab1/32/6",
          "controllingPort": 242,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core24",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core24",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 5
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
          "name": "fab1/32/7",
          "controllingPort": 243,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core24",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core24",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip32",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "244": {
        "mapping": {
          "id": 244,
          "name": "fab1/32/8",
          "controllingPort": 244,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core24",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core24",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "245": {
        "mapping": {
          "id": 245,
          "name": "fab1/33/1",
          "controllingPort": 245,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core25",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core25",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "246": {
        "mapping": {
          "id": 246,
          "name": "fab1/33/2",
          "controllingPort": 246,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core25",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core25",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "247": {
        "mapping": {
          "id": 247,
          "name": "fab1/33/3",
          "controllingPort": 247,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core25",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core25",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "248": {
        "mapping": {
          "id": 248,
          "name": "fab1/33/4",
          "controllingPort": 248,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core25",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "249": {
        "mapping": {
          "id": 249,
          "name": "fab1/33/5",
          "controllingPort": 249,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core25",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core25",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "250": {
        "mapping": {
          "id": 250,
          "name": "fab1/33/6",
          "controllingPort": 250,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core25",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core25",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 5
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
          "name": "fab1/33/7",
          "controllingPort": 251,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core25",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core25",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip33",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "252": {
        "mapping": {
          "id": 252,
          "name": "fab1/33/8",
          "controllingPort": 252,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core25",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core25",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "253": {
        "mapping": {
          "id": 253,
          "name": "fab1/34/1",
          "controllingPort": 253,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core26",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core26",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "254": {
        "mapping": {
          "id": 254,
          "name": "fab1/34/2",
          "controllingPort": 254,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core26",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core26",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "255": {
        "mapping": {
          "id": 255,
          "name": "fab1/34/3",
          "controllingPort": 255,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core26",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core26",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "256": {
        "mapping": {
          "id": 256,
          "name": "fab1/34/4",
          "controllingPort": 256,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core26",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "257": {
        "mapping": {
          "id": 257,
          "name": "fab1/34/5",
          "controllingPort": 257,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core26",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core26",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "258": {
        "mapping": {
          "id": 258,
          "name": "fab1/34/6",
          "controllingPort": 258,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core26",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core26",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 5
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
          "name": "fab1/34/7",
          "controllingPort": 259,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core26",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core26",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip34",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "260": {
        "mapping": {
          "id": 260,
          "name": "fab1/34/8",
          "controllingPort": 260,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core26",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core26",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "261": {
        "mapping": {
          "id": 261,
          "name": "fab1/35/1",
          "controllingPort": 261,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core27",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core27",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "262": {
        "mapping": {
          "id": 262,
          "name": "fab1/35/2",
          "controllingPort": 262,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core27",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core27",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "263": {
        "mapping": {
          "id": 263,
          "name": "fab1/35/3",
          "controllingPort": 263,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core27",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core27",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "264": {
        "mapping": {
          "id": 264,
          "name": "fab1/35/4",
          "controllingPort": 264,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core27",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "265": {
        "mapping": {
          "id": 265,
          "name": "fab1/35/5",
          "controllingPort": 265,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core27",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core27",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "266": {
        "mapping": {
          "id": 266,
          "name": "fab1/35/6",
          "controllingPort": 266,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core27",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core27",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 5
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
          "name": "fab1/35/7",
          "controllingPort": 267,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core27",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core27",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip35",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "268": {
        "mapping": {
          "id": 268,
          "name": "fab1/35/8",
          "controllingPort": 268,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core27",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core27",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "269": {
        "mapping": {
          "id": 269,
          "name": "fab1/36/1",
          "controllingPort": 269,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core28",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core28",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "270": {
        "mapping": {
          "id": 270,
          "name": "fab1/36/2",
          "controllingPort": 270,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core28",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core28",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "271": {
        "mapping": {
          "id": 271,
          "name": "fab1/36/3",
          "controllingPort": 271,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core28",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core28",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "272": {
        "mapping": {
          "id": 272,
          "name": "fab1/36/4",
          "controllingPort": 272,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core28",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "273": {
        "mapping": {
          "id": 273,
          "name": "fab1/36/5",
          "controllingPort": 273,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core28",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core28",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "274": {
        "mapping": {
          "id": 274,
          "name": "fab1/36/6",
          "controllingPort": 274,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core28",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core28",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 5
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
          "name": "fab1/36/7",
          "controllingPort": 275,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core28",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core28",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip36",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "276": {
        "mapping": {
          "id": 276,
          "name": "fab1/36/8",
          "controllingPort": 276,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core28",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core28",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "277": {
        "mapping": {
          "id": 277,
          "name": "fab1/37/1",
          "controllingPort": 277,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core29",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core29",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "278": {
        "mapping": {
          "id": 278,
          "name": "fab1/37/2",
          "controllingPort": 278,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core29",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core29",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "279": {
        "mapping": {
          "id": 279,
          "name": "fab1/37/3",
          "controllingPort": 279,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core29",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core29",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "280": {
        "mapping": {
          "id": 280,
          "name": "fab1/37/4",
          "controllingPort": 280,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core29",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "281": {
        "mapping": {
          "id": 281,
          "name": "fab1/37/5",
          "controllingPort": 281,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core29",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core29",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "282": {
        "mapping": {
          "id": 282,
          "name": "fab1/37/6",
          "controllingPort": 282,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core29",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core29",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 5
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
          "name": "fab1/37/7",
          "controllingPort": 283,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core29",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core29",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip37",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "284": {
        "mapping": {
          "id": 284,
          "name": "fab1/37/8",
          "controllingPort": 284,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core29",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core29",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "285": {
        "mapping": {
          "id": 285,
          "name": "fab1/38/1",
          "controllingPort": 285,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core30",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core30",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "286": {
        "mapping": {
          "id": 286,
          "name": "fab1/38/2",
          "controllingPort": 286,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core30",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core30",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "287": {
        "mapping": {
          "id": 287,
          "name": "fab1/38/3",
          "controllingPort": 287,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core30",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core30",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "288": {
        "mapping": {
          "id": 288,
          "name": "fab1/38/4",
          "controllingPort": 288,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core30",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "289": {
        "mapping": {
          "id": 289,
          "name": "fab1/38/5",
          "controllingPort": 289,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core30",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core30",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "290": {
        "mapping": {
          "id": 290,
          "name": "fab1/38/6",
          "controllingPort": 290,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core30",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core30",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 5
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
          "name": "fab1/38/7",
          "controllingPort": 291,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core30",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core30",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip38",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "292": {
        "mapping": {
          "id": 292,
          "name": "fab1/38/8",
          "controllingPort": 292,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core30",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core30",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "293": {
        "mapping": {
          "id": 293,
          "name": "fab1/39/1",
          "controllingPort": 293,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core31",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core31",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "294": {
        "mapping": {
          "id": 294,
          "name": "fab1/39/2",
          "controllingPort": 294,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core31",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core31",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "295": {
        "mapping": {
          "id": 295,
          "name": "fab1/39/3",
          "controllingPort": 295,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core31",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core31",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "296": {
        "mapping": {
          "id": 296,
          "name": "fab1/39/4",
          "controllingPort": 296,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core31",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "297": {
        "mapping": {
          "id": 297,
          "name": "fab1/39/5",
          "controllingPort": 297,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core31",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core31",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "298": {
        "mapping": {
          "id": 298,
          "name": "fab1/39/6",
          "controllingPort": 298,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core31",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core31",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 5
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
          "name": "fab1/39/7",
          "controllingPort": 299,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core31",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core31",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip39",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "300": {
        "mapping": {
          "id": 300,
          "name": "fab1/39/8",
          "controllingPort": 300,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core31",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core31",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "301": {
        "mapping": {
          "id": 301,
          "name": "fab1/40/1",
          "controllingPort": 301,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core32",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core32",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "302": {
        "mapping": {
          "id": 302,
          "name": "fab1/40/2",
          "controllingPort": 302,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core32",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core32",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "303": {
        "mapping": {
          "id": 303,
          "name": "fab1/40/3",
          "controllingPort": 303,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core32",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core32",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "304": {
        "mapping": {
          "id": 304,
          "name": "fab1/40/4",
          "controllingPort": 304,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core32",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core32",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "305": {
        "mapping": {
          "id": 305,
          "name": "fab1/40/5",
          "controllingPort": 305,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core32",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core32",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "306": {
        "mapping": {
          "id": 306,
          "name": "fab1/40/6",
          "controllingPort": 306,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core32",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core32",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 5
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
          "name": "fab1/40/7",
          "controllingPort": 307,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core32",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core32",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip40",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "308": {
        "mapping": {
          "id": 308,
          "name": "fab1/40/8",
          "controllingPort": 308,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core32",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core32",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "309": {
        "mapping": {
          "id": 309,
          "name": "eth1/41/1",
          "controllingPort": 309,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip1/core18",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-QSFP28-slot1/chip41",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip1/core18",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-QSFP28-slot1/chip41",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip1/core18",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-QSFP28-slot1/chip41",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip1/core18",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-QSFP28-slot1/chip41",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip1/core18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-QSFP28-slot1/chip41",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-QSFP28-slot1/chip41",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-QSFP28-slot1/chip41",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-QSFP28-slot1/chip41",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "310": {
        "mapping": {
          "id": 310,
          "name": "fab1/42/1",
          "controllingPort": 310,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core34",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core34",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "311": {
        "mapping": {
          "id": 311,
          "name": "fab1/42/2",
          "controllingPort": 311,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core34",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core34",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "312": {
        "mapping": {
          "id": 312,
          "name": "fab1/42/3",
          "controllingPort": 312,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core34",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core34",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "313": {
        "mapping": {
          "id": 313,
          "name": "fab1/42/4",
          "controllingPort": 313,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core34",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core34",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "314": {
        "mapping": {
          "id": 314,
          "name": "fab1/42/5",
          "controllingPort": 314,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core34",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core34",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 4
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
          "name": "fab1/42/6",
          "controllingPort": 315,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core34",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core34",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "316": {
        "mapping": {
          "id": 316,
          "name": "fab1/42/7",
          "controllingPort": 316,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core34",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core34",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip42",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "317": {
        "mapping": {
          "id": 317,
          "name": "fab1/42/8",
          "controllingPort": 317,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core34",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core34",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    },
    "318": {
        "mapping": {
          "id": 318,
          "name": "fab1/43/1",
          "controllingPort": 318,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core35",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core35",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "319": {
        "mapping": {
          "id": 319,
          "name": "fab1/43/2",
          "controllingPort": 319,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core35",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core35",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "320": {
        "mapping": {
          "id": 320,
          "name": "fab1/43/3",
          "controllingPort": 320,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core35",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core35",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "321": {
        "mapping": {
          "id": 321,
          "name": "fab1/43/4",
          "controllingPort": 321,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core35",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core35",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "322": {
        "mapping": {
          "id": 322,
          "name": "fab1/43/5",
          "controllingPort": 322,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core35",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core35",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 4
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
          "name": "fab1/43/6",
          "controllingPort": 323,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core35",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core35",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "324": {
        "mapping": {
          "id": 324,
          "name": "fab1/43/7",
          "controllingPort": 324,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core35",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core35",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-OSFP-slot1/chip43",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "325": {
        "mapping": {
          "id": 325,
          "name": "fab1/43/8",
          "controllingPort": 325,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core35",
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
          "portType": 1
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core35",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "326": {
        "mapping": {
          "id": 326,
          "name": "eth1/44/1",
          "controllingPort": 326,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core18",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-QSFP28-slot1/chip44",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core18",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-QSFP28-slot1/chip44",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core18",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-QSFP28-slot1/chip44",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-J3_NIF-slot1/chip2/core18",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-QSFP28-slot1/chip44",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_NIF-slot1/chip2/core18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-QSFP28-slot1/chip44",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-QSFP28-slot1/chip44",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-QSFP28-slot1/chip44",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-QSFP28-slot1/chip44",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "327": {
        "mapping": {
          "id": 327,
          "name": "eth1/45/1",
          "controllingPort": 327,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core37",
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
                "chip": "NPU-J3_FE-slot1/chip2/core37",
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
                "chip": "NPU-J3_FE-slot1/chip2/core37",
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
                "chip": "NPU-J3_FE-slot1/chip2/core37",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip45",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core37",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core37",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core37",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core37",
                      "lane": 3
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
                  }
                ]
              }
          }
        }
    },
    "328": {
        "mapping": {
          "id": 328,
          "name": "eth1/45/5",
          "controllingPort": 328,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core37",
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
                "chip": "NPU-J3_FE-slot1/chip2/core37",
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
                "chip": "NPU-J3_FE-slot1/chip2/core37",
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
                "chip": "NPU-J3_FE-slot1/chip2/core37",
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
          "portType": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core37",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core37",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core37",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core37",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    "329": {
        "mapping": {
          "id": 329,
          "name": "eth1/46/1",
          "controllingPort": 329,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core38",
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
                "chip": "NPU-J3_FE-slot1/chip2/core38",
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
                "chip": "NPU-J3_FE-slot1/chip2/core38",
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
                "chip": "NPU-J3_FE-slot1/chip2/core38",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-OSFP-slot1/chip46",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core38",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core38",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core38",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core38",
                      "lane": 3
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
                  }
                ]
              }
          }
        }
    },
    "330": {
        "mapping": {
          "id": 330,
          "name": "eth1/46/5",
          "controllingPort": 330,
          "pins": [
            {
              "a": {
                "chip": "NPU-J3_FE-slot1/chip2/core38",
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
                "chip": "NPU-J3_FE-slot1/chip2/core38",
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
                "chip": "NPU-J3_FE-slot1/chip2/core38",
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
                "chip": "NPU-J3_FE-slot1/chip2/core38",
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
          "portType": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core38",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core38",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core38",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-J3_FE-slot1/chip2/core38",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
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
    }
  },
  "chips": [
    {
      "name": "NPU-J3_NIF-slot1/chip1/core0",
      "type": 1,
      "physicalID": 0
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core0",
      "type": 1,
      "physicalID": 0
    },
    {
      "name": "NPU-J3_NIF-slot1/chip1/core1",
      "type": 1,
      "physicalID": 1
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core1",
      "type": 1,
      "physicalID": 1
    },
    {
      "name": "NPU-J3_NIF-slot1/chip1/core2",
      "type": 1,
      "physicalID": 2
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core2",
      "type": 1,
      "physicalID": 2
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core3",
      "type": 1,
      "physicalID": 3
    },
    {
      "name": "NPU-J3_NIF-slot1/chip1/core3",
      "type": 1,
      "physicalID": 3
    },
    {
      "name": "NPU-J3_NIF-slot1/chip1/core4",
      "type": 1,
      "physicalID": 4
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core4",
      "type": 1,
      "physicalID": 4
    },
    {
      "name": "NPU-J3_NIF-slot1/chip1/core5",
      "type": 1,
      "physicalID": 5
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core5",
      "type": 1,
      "physicalID": 5
    },
    {
      "name": "NPU-J3_NIF-slot1/chip1/core6",
      "type": 1,
      "physicalID": 6
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core6",
      "type": 1,
      "physicalID": 6
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core7",
      "type": 1,
      "physicalID": 7
    },
    {
      "name": "NPU-J3_NIF-slot1/chip1/core7",
      "type": 1,
      "physicalID": 7
    },
    {
      "name": "NPU-J3_NIF-slot1/chip1/core8",
      "type": 1,
      "physicalID": 8
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core8",
      "type": 1,
      "physicalID": 8
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core9",
      "type": 1,
      "physicalID": 9
    },
    {
      "name": "NPU-J3_NIF-slot1/chip1/core9",
      "type": 1,
      "physicalID": 9
    },
    {
      "name": "NPU-J3_NIF-slot1/chip1/core10",
      "type": 1,
      "physicalID": 10
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core10",
      "type": 1,
      "physicalID": 10
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core11",
      "type": 1,
      "physicalID": 11
    },
    {
      "name": "NPU-J3_NIF-slot1/chip1/core11",
      "type": 1,
      "physicalID": 11
    },
    {
      "name": "NPU-J3_NIF-slot1/chip1/core12",
      "type": 1,
      "physicalID": 12
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core12",
      "type": 1,
      "physicalID": 12
    },
    {
      "name": "NPU-J3_NIF-slot1/chip1/core13",
      "type": 1,
      "physicalID": 13
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core13",
      "type": 1,
      "physicalID": 13
    },
    {
      "name": "NPU-J3_NIF-slot1/chip1/core14",
      "type": 1,
      "physicalID": 14
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core14",
      "type": 1,
      "physicalID": 14
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core15",
      "type": 1,
      "physicalID": 15
    },
    {
      "name": "NPU-J3_NIF-slot1/chip1/core15",
      "type": 1,
      "physicalID": 15
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core16",
      "type": 1,
      "physicalID": 16
    },
    {
      "name": "NPU-J3_NIF-slot1/chip1/core16",
      "type": 1,
      "physicalID": 16
    },
    {
      "name": "NPU-J3_NIF-slot1/chip1/core17",
      "type": 1,
      "physicalID": 17
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core17",
      "type": 1,
      "physicalID": 17
    },
    {
      "name": "NPU-J3_NIF-slot1/chip1/core18",
      "type": 1,
      "physicalID": 18
    },
    {
      "name": "NPU-J3_NIF-slot1/chip2/core18",
      "type": 1,
      "physicalID": 18
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core19",
      "type": 1,
      "physicalID": 19
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core19",
      "type": 1,
      "physicalID": 19
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core20",
      "type": 1,
      "physicalID": 20
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core20",
      "type": 1,
      "physicalID": 20
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core21",
      "type": 1,
      "physicalID": 21
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core21",
      "type": 1,
      "physicalID": 21
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core22",
      "type": 1,
      "physicalID": 22
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core22",
      "type": 1,
      "physicalID": 22
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core23",
      "type": 1,
      "physicalID": 23
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core23",
      "type": 1,
      "physicalID": 23
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core24",
      "type": 1,
      "physicalID": 24
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core24",
      "type": 1,
      "physicalID": 24
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core25",
      "type": 1,
      "physicalID": 25
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core25",
      "type": 1,
      "physicalID": 25
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core26",
      "type": 1,
      "physicalID": 26
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core26",
      "type": 1,
      "physicalID": 26
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core27",
      "type": 1,
      "physicalID": 27
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core27",
      "type": 1,
      "physicalID": 27
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core28",
      "type": 1,
      "physicalID": 28
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core28",
      "type": 1,
      "physicalID": 28
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core29",
      "type": 1,
      "physicalID": 29
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core29",
      "type": 1,
      "physicalID": 29
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core30",
      "type": 1,
      "physicalID": 30
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core30",
      "type": 1,
      "physicalID": 30
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core31",
      "type": 1,
      "physicalID": 31
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core31",
      "type": 1,
      "physicalID": 31
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core32",
      "type": 1,
      "physicalID": 32
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core32",
      "type": 1,
      "physicalID": 32
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core33",
      "type": 1,
      "physicalID": 33
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core33",
      "type": 1,
      "physicalID": 33
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core34",
      "type": 1,
      "physicalID": 34
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core34",
      "type": 1,
      "physicalID": 34
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core35",
      "type": 1,
      "physicalID": 35
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core35",
      "type": 1,
      "physicalID": 35
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core36",
      "type": 1,
      "physicalID": 36
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core36",
      "type": 1,
      "physicalID": 36
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core37",
      "type": 1,
      "physicalID": 37
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core37",
      "type": 1,
      "physicalID": 37
    },
    {
      "name": "NPU-J3_FE-slot1/chip1/core38",
      "type": 1,
      "physicalID": 38
    },
    {
      "name": "NPU-J3_FE-slot1/chip2/core38",
      "type": 1,
      "physicalID": 38
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
      "name": "TRANSCEIVER-QSFP28-slot1/chip41",
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
      "name": "TRANSCEIVER-QSFP28-slot1/chip44",
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
    }
  ],
  "platformSupportedProfiles": [
    {
      "factor": {
        "profileID": 42
      },
      "profile": {
        "speed": 106250,
        "iphy": {
          "numLanes": 1,
          "modulation": 2,
          "fec": 11,
          "medium": 2,
          "interfaceType": 41
        }
      }
    },
    {
      "factor": {
        "profileID": 38
      },
      "profile": {
        "speed": 400000,
        "iphy": {
          "numLanes": 4,
          "modulation": 2,
          "fec": 11,
          "medium": 2,
          "interfaceType": 41
        }
      }
    },
    {
      "factor": {
        "profileID": 22
      },
      "profile": {
        "speed": 100000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 528,
          "medium": 1,
          "interfaceType": 41
        }
      }
    }
  ]
}
)";
} // namespace

namespace facebook::fboss {
Janga800bicPlatformMapping::Janga800bicPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

Janga800bicPlatformMapping::Janga800bicPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace facebook::fboss
