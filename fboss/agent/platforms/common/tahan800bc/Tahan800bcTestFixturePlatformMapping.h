/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

namespace {
constexpr auto kJsonTestFixturePlatformMappingStr = R"(
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core16",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core16",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core16",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core16",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core16",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core16",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core16",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core16",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core16",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core16",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core16",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core16",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
          },
          "39": {
              "subsumedPorts": [
                2
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core16",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core16",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core16",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core16",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core16",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core16",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core16",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core16",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "2": {
        "mapping": {
          "id": 2,
          "name": "eth1/1/5",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core16",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core16",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core16",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core16",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core16",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core16",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core16",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core16",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core17",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core17",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core17",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core17",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core17",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core17",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core17",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core17",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core17",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core17",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core17",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core17",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
          },
          "39": {
              "subsumedPorts": [
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core17",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core17",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core17",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core17",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core17",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core17",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core17",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core17",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "4": {
        "mapping": {
          "id": 4,
          "name": "eth1/2/5",
          "controllingPort": 3,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core17",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core17",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core17",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core17",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core17",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core17",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core17",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core17",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
          "name": "eth1/3/1",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core18",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core18",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core18",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core18",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core18",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core18",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core18",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core18",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core18",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core18",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core18",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core18",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                6
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core18",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core18",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core18",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core18",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core18",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core18",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core18",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core18",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "6": {
        "mapping": {
          "id": 6,
          "name": "eth1/3/5",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core18",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core18",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core18",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core18",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core18",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core18",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core18",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core18",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "7": {
        "mapping": {
          "id": 7,
          "name": "eth1/4/1",
          "controllingPort": 7,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core19",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core19",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core19",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core19",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core19",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core19",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core19",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core19",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core19",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core19",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core19",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core19",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core19",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core19",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core19",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core19",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core19",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core19",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core19",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core19",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "8": {
        "mapping": {
          "id": 8,
          "name": "eth1/4/5",
          "controllingPort": 7,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core19",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core19",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core19",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core19",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core19",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core19",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core19",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core19",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "9": {
        "mapping": {
          "id": 9,
          "name": "eth1/5/1",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core20",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core20",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core20",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core20",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core20",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core20",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core20",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core20",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core20",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core20",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core20",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core20",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                10
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core20",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core20",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core20",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core20",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core20",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core20",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core20",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core20",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "10": {
        "mapping": {
          "id": 10,
          "name": "eth1/5/5",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core20",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core20",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core20",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core20",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core20",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core20",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core20",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core20",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "11": {
        "mapping": {
          "id": 11,
          "name": "eth1/6/1",
          "controllingPort": 11,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core21",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core21",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core21",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core21",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core21",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core21",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core21",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core21",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core21",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core21",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core21",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core21",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core21",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core21",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core21",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core21",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core21",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core21",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core21",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core21",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "12": {
        "mapping": {
          "id": 12,
          "name": "eth1/6/5",
          "controllingPort": 11,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core21",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core21",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core21",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core21",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core21",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core21",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core21",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core21",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "13": {
        "mapping": {
          "id": 13,
          "name": "eth1/7/1",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core22",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core22",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core22",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core22",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core22",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core22",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core22",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core22",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core22",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core22",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core22",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core22",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                14
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core22",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core22",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core22",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core22",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core22",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core22",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core22",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core22",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "14": {
        "mapping": {
          "id": 14,
          "name": "eth1/7/5",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core22",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core22",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core22",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core22",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core22",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core22",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core22",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core22",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "15": {
        "mapping": {
          "id": 15,
          "name": "eth1/8/1",
          "controllingPort": 15,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core23",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core23",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core23",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core23",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core23",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core23",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core23",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core23",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core23",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core23",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core23",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core23",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core23",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core23",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core23",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core23",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core23",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core23",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core23",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core23",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "16": {
        "mapping": {
          "id": 16,
          "name": "eth1/8/5",
          "controllingPort": 15,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core23",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core23",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core23",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core23",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core23",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core23",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core23",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core23",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "17": {
        "mapping": {
          "id": 17,
          "name": "eth1/9/1",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core24",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core24",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core24",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core24",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core24",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core24",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core24",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core24",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core24",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core24",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core24",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core24",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                18
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core24",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core24",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core24",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core24",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core24",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core24",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core24",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core24",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "18": {
        "mapping": {
          "id": 18,
          "name": "eth1/9/5",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core24",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core24",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core24",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core24",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core24",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core24",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core24",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core24",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "19": {
        "mapping": {
          "id": 19,
          "name": "eth1/10/1",
          "controllingPort": 19,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core25",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core25",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core25",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core25",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core25",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core25",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core25",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core25",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core25",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core25",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core25",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core25",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core25",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core25",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core25",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core25",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core25",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core25",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core25",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core25",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "20": {
        "mapping": {
          "id": 20,
          "name": "eth1/10/5",
          "controllingPort": 19,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core25",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core25",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core25",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core25",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core25",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core25",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core25",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core25",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "21": {
        "mapping": {
          "id": 21,
          "name": "eth1/11/1",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core26",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core26",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core26",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core26",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core26",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core26",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core26",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core26",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core26",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core26",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core26",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core26",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                22
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core26",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core26",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core26",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core26",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core26",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core26",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core26",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core26",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "22": {
        "mapping": {
          "id": 22,
          "name": "eth1/11/5",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core26",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core26",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core26",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core26",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core26",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core26",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core26",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core26",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "23": {
        "mapping": {
          "id": 23,
          "name": "eth1/12/1",
          "controllingPort": 23,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core27",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core27",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core27",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core27",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core27",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core27",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core27",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core27",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core27",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core27",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core27",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core27",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core27",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core27",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core27",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core27",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core27",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core27",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core27",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core27",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "24": {
        "mapping": {
          "id": 24,
          "name": "eth1/12/5",
          "controllingPort": 23,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core27",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core27",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core27",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core27",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core27",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core27",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core27",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core27",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "25": {
        "mapping": {
          "id": 25,
          "name": "eth1/13/1",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core28",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core28",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core28",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core28",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core28",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core28",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core28",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core28",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core28",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core28",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core28",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core28",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                26
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core28",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core28",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core28",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core28",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core28",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core28",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core28",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core28",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "26": {
        "mapping": {
          "id": 26,
          "name": "eth1/13/5",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core28",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core28",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core28",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core28",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core28",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core28",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core28",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core28",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "27": {
        "mapping": {
          "id": 27,
          "name": "eth1/14/1",
          "controllingPort": 27,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core29",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core29",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core29",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core29",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core29",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core29",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core29",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core29",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core29",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core29",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core29",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core29",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core29",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core29",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core29",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core29",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core29",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core29",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core29",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core29",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "28": {
        "mapping": {
          "id": 28,
          "name": "eth1/14/5",
          "controllingPort": 27,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core29",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core29",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core29",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core29",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core29",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core29",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core29",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core29",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "29": {
        "mapping": {
          "id": 29,
          "name": "eth1/15/1",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core30",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core30",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core30",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core30",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core30",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core30",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core30",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core30",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core30",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core30",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core30",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core30",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                30
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core30",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core30",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core30",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core30",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core30",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core30",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core30",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core30",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "30": {
        "mapping": {
          "id": 30,
          "name": "eth1/15/5",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core30",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core30",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core30",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core30",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core30",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core30",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core30",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core30",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "31": {
        "mapping": {
          "id": 31,
          "name": "eth1/16/1",
          "controllingPort": 31,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core31",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core31",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core31",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core31",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core31",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core31",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core31",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core31",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core31",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core31",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core31",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core31",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core31",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core31",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core31",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core31",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core31",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core31",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core31",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core31",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "32": {
        "mapping": {
          "id": 32,
          "name": "eth1/16/5",
          "controllingPort": 31,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core31",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core31",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core31",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core31",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core31",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core31",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core31",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core31",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "33": {
        "mapping": {
          "id": 33,
          "name": "eth1/17/1",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core32",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core32",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core32",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core32",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core32",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core32",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core32",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core32",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core32",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core32",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core32",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core32",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                34
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core32",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core32",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core32",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core32",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core32",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core32",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core32",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core32",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "34": {
        "mapping": {
          "id": 34,
          "name": "eth1/17/5",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core32",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core32",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core32",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core32",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core32",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core32",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core32",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core32",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "35": {
        "mapping": {
          "id": 35,
          "name": "eth1/18/1",
          "controllingPort": 35,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core33",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core33",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core33",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core33",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core33",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core33",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core33",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core33",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core33",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core33",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core33",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core33",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                36
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core33",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core33",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core33",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core33",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core33",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core33",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core33",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core33",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "36": {
        "mapping": {
          "id": 36,
          "name": "eth1/18/5",
          "controllingPort": 35,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core33",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core33",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core33",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core33",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core33",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core33",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core33",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core33",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "37": {
        "mapping": {
          "id": 37,
          "name": "eth1/19/1",
          "controllingPort": 37,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core34",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core34",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core34",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core34",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core34",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core34",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core34",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core34",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core34",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core34",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core34",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core34",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                38
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core34",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core34",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core34",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core34",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core34",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core34",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core34",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core34",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "38": {
        "mapping": {
          "id": 38,
          "name": "eth1/19/5",
          "controllingPort": 37,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core34",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core34",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core34",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core34",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core34",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core34",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core34",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core34",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "39": {
        "mapping": {
          "id": 39,
          "name": "eth1/20/1",
          "controllingPort": 39,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core35",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core35",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core35",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core35",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core35",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core35",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core35",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core35",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core35",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core35",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core35",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core35",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                40
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core35",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core35",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core35",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core35",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core35",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core35",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core35",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core35",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "40": {
        "mapping": {
          "id": 40,
          "name": "eth1/20/5",
          "controllingPort": 39,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core35",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core35",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core35",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core35",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core35",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core35",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core35",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core35",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 128,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "41": {
        "mapping": {
          "id": 41,
          "name": "eth1/21/1",
          "controllingPort": 41,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core36",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core36",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core36",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core36",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core36",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core36",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core36",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core36",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core36",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core36",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core36",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core36",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                42
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core36",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core36",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core36",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core36",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core36",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core36",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core36",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core36",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "42": {
        "mapping": {
          "id": 42,
          "name": "eth1/21/5",
          "controllingPort": 41,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core36",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core36",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core36",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core36",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core36",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core36",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core36",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core36",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "43": {
        "mapping": {
          "id": 43,
          "name": "eth1/22/1",
          "controllingPort": 43,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core37",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core37",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core37",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core37",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core37",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core37",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core37",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core37",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core37",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core37",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core37",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core37",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                44
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core37",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core37",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core37",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core37",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core37",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core37",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core37",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core37",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "44": {
        "mapping": {
          "id": 44,
          "name": "eth1/22/5",
          "controllingPort": 43,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core37",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core37",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core37",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core37",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core37",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core37",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core37",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core37",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "45": {
        "mapping": {
          "id": 45,
          "name": "eth1/23/1",
          "controllingPort": 45,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core38",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core38",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core38",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core38",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core38",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core38",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core38",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core38",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core38",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core38",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core38",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core38",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                46
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core38",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core38",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core38",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core38",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core38",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core38",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core38",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core38",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "46": {
        "mapping": {
          "id": 46,
          "name": "eth1/23/5",
          "controllingPort": 45,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core38",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core38",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core38",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core38",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core38",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core38",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core38",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core38",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "47": {
        "mapping": {
          "id": 47,
          "name": "eth1/24/1",
          "controllingPort": 47,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core39",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core39",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core39",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core39",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core39",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core39",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core39",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core39",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core39",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core39",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core39",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core39",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                48
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core39",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core39",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core39",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core39",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core39",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core39",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core39",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core39",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "48": {
        "mapping": {
          "id": 48,
          "name": "eth1/24/5",
          "controllingPort": 47,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core39",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core39",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core39",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core39",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core39",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core39",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core39",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core39",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "49": {
        "mapping": {
          "id": 49,
          "name": "eth1/25/1",
          "controllingPort": 49,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core40",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core40",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core40",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core40",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core40",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core40",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core40",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core40",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core40",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core40",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core40",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core40",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                50
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core40",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core40",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core40",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core40",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core40",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core40",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core40",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core40",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "50": {
        "mapping": {
          "id": 50,
          "name": "eth1/25/5",
          "controllingPort": 49,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core40",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core40",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core40",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core40",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core40",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core40",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core40",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core40",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "51": {
        "mapping": {
          "id": 51,
          "name": "eth1/26/1",
          "controllingPort": 51,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core41",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core41",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core41",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core41",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core41",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core41",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core41",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core41",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core41",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core41",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core41",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core41",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                52
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core41",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core41",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core41",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core41",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core41",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core41",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core41",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core41",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "52": {
        "mapping": {
          "id": 52,
          "name": "eth1/26/5",
          "controllingPort": 51,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core41",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core41",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core41",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core41",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core41",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core41",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core41",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core41",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "53": {
        "mapping": {
          "id": 53,
          "name": "eth1/27/1",
          "controllingPort": 53,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core42",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core42",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core42",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core42",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core42",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core42",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core42",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core42",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core42",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core42",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core42",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core42",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                54
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core42",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core42",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core42",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core42",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core42",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core42",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core42",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core42",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "54": {
        "mapping": {
          "id": 54,
          "name": "eth1/27/5",
          "controllingPort": 53,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core42",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core42",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core42",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core42",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core42",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core42",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core42",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core42",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "55": {
        "mapping": {
          "id": 55,
          "name": "eth1/28/1",
          "controllingPort": 55,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core43",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core43",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core43",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core43",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core43",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core43",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core43",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core43",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core43",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core43",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core43",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core43",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                56
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core43",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core43",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core43",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core43",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core43",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core43",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core43",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core43",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "56": {
        "mapping": {
          "id": 56,
          "name": "eth1/28/5",
          "controllingPort": 55,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core43",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core43",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core43",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core43",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core43",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core43",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core43",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core43",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "57": {
        "mapping": {
          "id": 57,
          "name": "eth1/29/1",
          "controllingPort": 57,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core44",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core44",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core44",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core44",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core44",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core44",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core44",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core44",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core44",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core44",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core44",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core44",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                58
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core44",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core44",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core44",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core44",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core44",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core44",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core44",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core44",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "58": {
        "mapping": {
          "id": 58,
          "name": "eth1/29/5",
          "controllingPort": 57,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core44",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core44",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core44",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core44",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core44",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core44",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core44",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core44",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "59": {
        "mapping": {
          "id": 59,
          "name": "eth1/30/1",
          "controllingPort": 59,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core45",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core45",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core45",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core45",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core45",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core45",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core45",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core45",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core45",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core45",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core45",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core45",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                60
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core45",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core45",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core45",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core45",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core45",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core45",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core45",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core45",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "60": {
        "mapping": {
          "id": 60,
          "name": "eth1/30/5",
          "controllingPort": 59,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core45",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core45",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core45",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core45",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core45",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core45",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core45",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core45",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "61": {
        "mapping": {
          "id": 61,
          "name": "eth1/31/1",
          "controllingPort": 61,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core46",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core46",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core46",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core46",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core46",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core46",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core46",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core46",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core46",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core46",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core46",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core46",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                62
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core46",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core46",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core46",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core46",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core46",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core46",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core46",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core46",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "62": {
        "mapping": {
          "id": 62,
          "name": "eth1/31/5",
          "controllingPort": 61,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core46",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core46",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core46",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core46",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core46",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core46",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core46",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core46",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "63": {
        "mapping": {
          "id": 63,
          "name": "eth1/32/1",
          "controllingPort": 63,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core47",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core47",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core47",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core47",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core47",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core47",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core47",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core47",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core47",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core47",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core47",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core47",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                64
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core47",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core47",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core47",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core47",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core47",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core47",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core47",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core47",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
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
    "64": {
        "mapping": {
          "id": 64,
          "name": "eth1/32/5",
          "controllingPort": 63,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core47",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core47",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core47",
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
                "chip": "NPU-TH5_NIF-slot1/chip1/core47",
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
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core47",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core47",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core47",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core47",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -28,
                      "pre2": 4,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
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
    "65": {
        "mapping": {
          "id": 65,
          "name": "eth1/33/1",
          "controllingPort": 65,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core64",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-QSFP28-slot1/chip33",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core64",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-QSFP28-slot1/chip33",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core64",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-QSFP28-slot1/chip33",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core64",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "TRANSCEIVER-QSFP28-slot1/chip33",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 4,
          "scope": 0
        },
        "supportedProfiles": {
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core64",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 5,
                      "pre2": 0,
                      "main": 31,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core64",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 5,
                      "pre2": 0,
                      "main": 31,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core64",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 5,
                      "pre2": 0,
                      "main": 31,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core64",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 5,
                      "pre2": 0,
                      "main": 31,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "TRANSCEIVER-QSFP28-slot1/chip33",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-QSFP28-slot1/chip33",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-QSFP28-slot1/chip33",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "TRANSCEIVER-QSFP28-slot1/chip33",
                      "lane": 3
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
          "name": "eth1/34/1",
          "controllingPort": 66,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core0",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip34",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip34",
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
          "name": "eth1/35/1",
          "controllingPort": 67,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core1",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip35",
                      "lane": 7
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
          "name": "eth1/36/1",
          "controllingPort": 68,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core2",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip36",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip36",
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
          "name": "eth1/37/1",
          "controllingPort": 69,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core3",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip37",
                      "lane": 7
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
          "name": "eth1/38/1",
          "controllingPort": 70,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core4",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip38",
                      "lane": 7
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
          "name": "eth1/39/1",
          "controllingPort": 71,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core5",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip39",
                      "lane": 7
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
          "name": "eth1/40/1",
          "controllingPort": 72,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core6",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip40",
                      "lane": 7
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
          "name": "eth1/41/1",
          "controllingPort": 73,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core7",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip41",
                      "lane": 7
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
          "name": "eth1/42/1",
          "controllingPort": 74,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core8",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip42",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip42",
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
          "name": "eth1/43/1",
          "controllingPort": 75,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core9",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip43",
                      "lane": 7
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
          "name": "eth1/44/1",
          "controllingPort": 76,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core10",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip44",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip44",
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
          "name": "eth1/45/1",
          "controllingPort": 77,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core11",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip45",
                      "lane": 7
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
          "name": "eth1/46/1",
          "controllingPort": 78,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core12",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip46",
                      "lane": 7
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
          "name": "eth1/47/1",
          "controllingPort": 79,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core13",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip47",
                      "lane": 7
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
          "name": "eth1/48/1",
          "controllingPort": 80,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core14",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip48",
                      "lane": 7
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
          "name": "eth1/49/1",
          "controllingPort": 81,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core15",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip49",
                      "lane": 7
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
          "name": "eth1/50/1",
          "controllingPort": 82,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core48",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip50",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip50",
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
          "name": "eth1/51/1",
          "controllingPort": 83,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core49",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip51",
                      "lane": 7
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
          "name": "eth1/52/1",
          "controllingPort": 84,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core50",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip52",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip52",
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
          "name": "eth1/53/1",
          "controllingPort": 85,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core51",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip53",
                      "lane": 7
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
          "name": "eth1/54/1",
          "controllingPort": 86,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core52",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip54",
                      "lane": 7
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
          "name": "eth1/55/1",
          "controllingPort": 87,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core53",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip55",
                      "lane": 7
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
          "name": "eth1/56/1",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core54",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip56",
                      "lane": 7
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
          "name": "eth1/57/1",
          "controllingPort": 89,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core55",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip57",
                      "lane": 7
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
          "name": "eth1/58/1",
          "controllingPort": 90,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core56",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip58",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip58",
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
          "name": "eth1/59/1",
          "controllingPort": 91,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core57",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip59",
                      "lane": 7
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
          "name": "eth1/60/1",
          "controllingPort": 92,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core58",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip60",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip60",
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
          "name": "eth1/61/1",
          "controllingPort": 93,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core59",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip61",
                      "lane": 7
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
          "name": "eth1/62/1",
          "controllingPort": 94,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core60",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip62",
                      "lane": 7
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
          "name": "eth1/63/1",
          "controllingPort": 95,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core61",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip63",
                      "lane": 7
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
          "name": "eth1/64/1",
          "controllingPort": 96,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core62",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip64",
                      "lane": 7
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
          "name": "eth1/65/1",
          "controllingPort": 97,
          "pins": [
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0,
          "scope": 0
        },
        "supportedProfiles": {
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                      "lane": 3
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
                      "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                      "lane": 4
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                      "lane": 5
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                      "lane": 6
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "NPU-TH5_NIF-slot1/chip1/core63",
                      "lane": 7
                    },
                    "tx": {
                      "pre": 0,
                      "pre2": 0,
                      "main": 168,
                      "post": 0,
                      "post2": 0,
                      "post3": 0,
                      "pre3": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip65",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BACKPLANE-EXAMAX-slot1/chip65",
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
      "name": "NPU-TH5_NIF-slot1/chip1/core0",
      "type": 1,
      "physicalID": 0
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core1",
      "type": 1,
      "physicalID": 1
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core2",
      "type": 1,
      "physicalID": 2
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core3",
      "type": 1,
      "physicalID": 3
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core4",
      "type": 1,
      "physicalID": 4
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core5",
      "type": 1,
      "physicalID": 5
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core6",
      "type": 1,
      "physicalID": 6
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core7",
      "type": 1,
      "physicalID": 7
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core8",
      "type": 1,
      "physicalID": 8
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core9",
      "type": 1,
      "physicalID": 9
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core10",
      "type": 1,
      "physicalID": 10
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core11",
      "type": 1,
      "physicalID": 11
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core12",
      "type": 1,
      "physicalID": 12
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core13",
      "type": 1,
      "physicalID": 13
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core14",
      "type": 1,
      "physicalID": 14
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core15",
      "type": 1,
      "physicalID": 15
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core16",
      "type": 1,
      "physicalID": 16
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core17",
      "type": 1,
      "physicalID": 17
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core18",
      "type": 1,
      "physicalID": 18
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core19",
      "type": 1,
      "physicalID": 19
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core20",
      "type": 1,
      "physicalID": 20
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core21",
      "type": 1,
      "physicalID": 21
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core22",
      "type": 1,
      "physicalID": 22
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core23",
      "type": 1,
      "physicalID": 23
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core24",
      "type": 1,
      "physicalID": 24
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core25",
      "type": 1,
      "physicalID": 25
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core26",
      "type": 1,
      "physicalID": 26
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core27",
      "type": 1,
      "physicalID": 27
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core28",
      "type": 1,
      "physicalID": 28
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core29",
      "type": 1,
      "physicalID": 29
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core30",
      "type": 1,
      "physicalID": 30
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core31",
      "type": 1,
      "physicalID": 31
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core32",
      "type": 1,
      "physicalID": 32
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core33",
      "type": 1,
      "physicalID": 33
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core34",
      "type": 1,
      "physicalID": 34
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core35",
      "type": 1,
      "physicalID": 35
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core36",
      "type": 1,
      "physicalID": 36
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core37",
      "type": 1,
      "physicalID": 37
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core38",
      "type": 1,
      "physicalID": 38
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core39",
      "type": 1,
      "physicalID": 39
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core40",
      "type": 1,
      "physicalID": 40
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core41",
      "type": 1,
      "physicalID": 41
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core42",
      "type": 1,
      "physicalID": 42
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core43",
      "type": 1,
      "physicalID": 43
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core44",
      "type": 1,
      "physicalID": 44
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core45",
      "type": 1,
      "physicalID": 45
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core46",
      "type": 1,
      "physicalID": 46
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core47",
      "type": 1,
      "physicalID": 47
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core48",
      "type": 1,
      "physicalID": 48
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core49",
      "type": 1,
      "physicalID": 49
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core50",
      "type": 1,
      "physicalID": 50
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core51",
      "type": 1,
      "physicalID": 51
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core52",
      "type": 1,
      "physicalID": 52
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core53",
      "type": 1,
      "physicalID": 53
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core54",
      "type": 1,
      "physicalID": 54
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core55",
      "type": 1,
      "physicalID": 55
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core56",
      "type": 1,
      "physicalID": 56
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core57",
      "type": 1,
      "physicalID": 57
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core58",
      "type": 1,
      "physicalID": 58
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core59",
      "type": 1,
      "physicalID": 59
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core60",
      "type": 1,
      "physicalID": 60
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core61",
      "type": 1,
      "physicalID": 61
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core62",
      "type": 1,
      "physicalID": 62
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core63",
      "type": 1,
      "physicalID": 63
    },
    {
      "name": "NPU-TH5_NIF-slot1/chip1/core64",
      "type": 1,
      "physicalID": 64
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
      "name": "TRANSCEIVER-QSFP28-slot1/chip33",
      "type": 3,
      "physicalID": 32
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip34",
      "type": 4,
      "physicalID": 33
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip35",
      "type": 4,
      "physicalID": 34
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip36",
      "type": 4,
      "physicalID": 35
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip37",
      "type": 4,
      "physicalID": 36
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip38",
      "type": 4,
      "physicalID": 37
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip39",
      "type": 4,
      "physicalID": 38
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip40",
      "type": 4,
      "physicalID": 39
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip41",
      "type": 4,
      "physicalID": 40
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip42",
      "type": 4,
      "physicalID": 41
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip43",
      "type": 4,
      "physicalID": 42
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip44",
      "type": 4,
      "physicalID": 43
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip45",
      "type": 4,
      "physicalID": 44
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip46",
      "type": 4,
      "physicalID": 45
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip47",
      "type": 4,
      "physicalID": 46
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip48",
      "type": 4,
      "physicalID": 47
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip49",
      "type": 4,
      "physicalID": 48
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip50",
      "type": 4,
      "physicalID": 49
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip51",
      "type": 4,
      "physicalID": 50
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip52",
      "type": 4,
      "physicalID": 51
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip53",
      "type": 4,
      "physicalID": 52
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip54",
      "type": 4,
      "physicalID": 53
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip55",
      "type": 4,
      "physicalID": 54
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip56",
      "type": 4,
      "physicalID": 55
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip57",
      "type": 4,
      "physicalID": 56
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip58",
      "type": 4,
      "physicalID": 57
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip59",
      "type": 4,
      "physicalID": 58
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip60",
      "type": 4,
      "physicalID": 59
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip61",
      "type": 4,
      "physicalID": 60
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip62",
      "type": 4,
      "physicalID": 61
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip63",
      "type": 4,
      "physicalID": 62
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip64",
      "type": 4,
      "physicalID": 63
    },
    {
      "name": "BACKPLANE-EXAMAX-slot1/chip65",
      "type": 4,
      "physicalID": 64
    }
  ],
  "platformSupportedProfiles": [
    {
      "factor": {
        "profileID": 23
      },
      "profile": {
        "speed": 100000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 528,
          "medium": 2,
          "interfaceType": 21
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
          "interfaceType": 3
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
          "interfaceType": 4
        }
      }
    }
  ]
}
)";
} // namespace
