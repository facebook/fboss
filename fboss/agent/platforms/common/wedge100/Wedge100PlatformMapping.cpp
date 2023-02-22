/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/wedge100/Wedge100PlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{
  "ports": {
    "1": {
        "mapping": {
          "id": 1,
          "name": "eth1/5/1",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "FC1",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                2
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                2,
                3,
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                2,
                3,
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                2
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                2
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                2,
                3,
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                2,
                3,
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                2
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                2
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                2,
                3,
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
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
          "name": "eth1/5/2",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "FC1",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
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
          "name": "eth1/5/3",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "FC1",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
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
          "name": "eth1/5/4",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "FC1",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
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
          "name": "eth1/6/1",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "FC0",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                6
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                6,
                7,
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                6,
                7,
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                6
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                6
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                6,
                7,
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                6,
                7,
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                6
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                6
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                6,
                7,
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
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
          "name": "eth1/6/2",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "FC0",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
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
          "name": "eth1/6/3",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "FC0",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
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
          "name": "eth1/6/4",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "FC0",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
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
          "name": "eth1/7/1",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "FC3",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                10
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                10,
                11,
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                10,
                11,
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                10
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                10
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                10,
                11,
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                10,
                11,
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                10
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                10
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                10,
                11,
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
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
          "name": "eth1/7/2",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "FC3",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
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
          "name": "eth1/7/3",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "FC3",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
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
          "name": "eth1/7/4",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "FC3",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
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
          "name": "eth1/8/1",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "FC2",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                14
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                14,
                15,
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                14,
                15,
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                14
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                14
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                14,
                15,
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                14,
                15,
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                14
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                14
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                14,
                15,
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
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
          "name": "eth1/8/2",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "FC2",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
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
          "name": "eth1/8/3",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "FC2",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
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
          "name": "eth1/8/4",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "FC2",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
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
          "name": "eth1/9/1",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "FC5",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                18
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                18,
                19,
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                18,
                19,
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                18
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                18
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                18,
                19,
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                18,
                19,
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                18
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                18
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                18,
                19,
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
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
          "name": "eth1/9/2",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "FC5",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
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
          "name": "eth1/9/3",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "FC5",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
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
          "name": "eth1/9/4",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "FC5",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
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
          "name": "eth1/10/1",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "FC4",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                22
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                22,
                23,
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                22,
                23,
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                22
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                22
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                22,
                23,
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                22,
                23,
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                22
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                22
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                22,
                23,
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
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
          "name": "eth1/10/2",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "FC4",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
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
          "name": "eth1/10/3",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "FC4",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
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
          "name": "eth1/10/4",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "FC4",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
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
          "name": "eth1/11/1",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "FC7",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                26
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                26,
                27,
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                26,
                27,
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                26
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                26
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                26,
                27,
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                26,
                27,
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                26
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                26
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                26,
                27,
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
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
          "name": "eth1/11/2",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "FC7",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
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
          "name": "eth1/11/3",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "FC7",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
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
          "name": "eth1/11/4",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "FC7",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
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
          "name": "eth1/12/1",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "FC6",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                30
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                30,
                31,
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                30,
                31,
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                30
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                30
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                30,
                31,
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                30,
                31,
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                30
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                30
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                30,
                31,
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
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
          "name": "eth1/12/2",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "FC6",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
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
          "name": "eth1/12/3",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "FC6",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
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
          "name": "eth1/12/4",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "FC6",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
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
          "name": "eth1/13/1",
          "controllingPort": 34,
          "pins": [
            {
              "a": {
                "chip": "FC9",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                35
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                35,
                36,
                37
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                35,
                36,
                37
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                35
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                35
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                35,
                36,
                37
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                35,
                36,
                37
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                35
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                35
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                35,
                36,
                37
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
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
          "name": "eth1/13/2",
          "controllingPort": 34,
          "pins": [
            {
              "a": {
                "chip": "FC9",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
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
          "name": "eth1/13/3",
          "controllingPort": 34,
          "pins": [
            {
              "a": {
                "chip": "FC9",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                37
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                37
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                37
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                37
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                37
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
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
          "name": "eth1/13/4",
          "controllingPort": 34,
          "pins": [
            {
              "a": {
                "chip": "FC9",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
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
          "name": "eth1/14/1",
          "controllingPort": 38,
          "pins": [
            {
              "a": {
                "chip": "FC8",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                39
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                39,
                40,
                41
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                39,
                40,
                41
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                39
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                39
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                39,
                40,
                41
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                39,
                40,
                41
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                39
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                39
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                39,
                40,
                41
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
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
          "name": "eth1/14/2",
          "controllingPort": 38,
          "pins": [
            {
              "a": {
                "chip": "FC8",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
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
          "name": "eth1/14/3",
          "controllingPort": 38,
          "pins": [
            {
              "a": {
                "chip": "FC8",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                41
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                41
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                41
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                41
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                41
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
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
          "name": "eth1/14/4",
          "controllingPort": 38,
          "pins": [
            {
              "a": {
                "chip": "FC8",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
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
          "name": "eth1/15/1",
          "controllingPort": 42,
          "pins": [
            {
              "a": {
                "chip": "FC11",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                43
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                43,
                44,
                45
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                43,
                44,
                45
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                43
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                43
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                43,
                44,
                45
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                43,
                44,
                45
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                43
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                43
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                43,
                44,
                45
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
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
          "name": "eth1/15/2",
          "controllingPort": 42,
          "pins": [
            {
              "a": {
                "chip": "FC11",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
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
          "name": "eth1/15/3",
          "controllingPort": 42,
          "pins": [
            {
              "a": {
                "chip": "FC11",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                45
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                45
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                45
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                45
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                45
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
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
          "name": "eth1/15/4",
          "controllingPort": 42,
          "pins": [
            {
              "a": {
                "chip": "FC11",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
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
          "name": "eth1/16/1",
          "controllingPort": 46,
          "pins": [
            {
              "a": {
                "chip": "FC10",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                47
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                47,
                48,
                49
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                47,
                48,
                49
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                47
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                47
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                47,
                48,
                49
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                47,
                48,
                49
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                47
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                47
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                47,
                48,
                49
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
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
          "name": "eth1/16/2",
          "controllingPort": 46,
          "pins": [
            {
              "a": {
                "chip": "FC10",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
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
          "name": "eth1/16/3",
          "controllingPort": 46,
          "pins": [
            {
              "a": {
                "chip": "FC10",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                49
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                49
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                49
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                49
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                49
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
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
          "name": "eth1/16/4",
          "controllingPort": 46,
          "pins": [
            {
              "a": {
                "chip": "FC10",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
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
          "name": "eth1/17/1",
          "controllingPort": 50,
          "pins": [
            {
              "a": {
                "chip": "FC13",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                51
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                51,
                52,
                53
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                51,
                52,
                53
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                51
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                51
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                51,
                52,
                53
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                51,
                52,
                53
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                51
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                51
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                51,
                52,
                53
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
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
          "name": "eth1/17/2",
          "controllingPort": 50,
          "pins": [
            {
              "a": {
                "chip": "FC13",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
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
          "name": "eth1/17/3",
          "controllingPort": 50,
          "pins": [
            {
              "a": {
                "chip": "FC13",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                53
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                53
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                53
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                53
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                53
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
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
          "name": "eth1/17/4",
          "controllingPort": 50,
          "pins": [
            {
              "a": {
                "chip": "FC13",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
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
          "name": "eth1/18/1",
          "controllingPort": 54,
          "pins": [
            {
              "a": {
                "chip": "FC12",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                55
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                55,
                56,
                57
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                55,
                56,
                57
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                55
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                55
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                55,
                56,
                57
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                55,
                56,
                57
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                55
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                55
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                55,
                56,
                57
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
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
          "name": "eth1/18/2",
          "controllingPort": 54,
          "pins": [
            {
              "a": {
                "chip": "FC12",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
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
          "name": "eth1/18/3",
          "controllingPort": 54,
          "pins": [
            {
              "a": {
                "chip": "FC12",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                57
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                57
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                57
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                57
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                57
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
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
          "name": "eth1/18/4",
          "controllingPort": 54,
          "pins": [
            {
              "a": {
                "chip": "FC12",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
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
          "name": "eth1/19/1",
          "controllingPort": 58,
          "pins": [
            {
              "a": {
                "chip": "FC15",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                59
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                59,
                60,
                61
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                59,
                60,
                61
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                59
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                59
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                59,
                60,
                61
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                59,
                60,
                61
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                59
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                59
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                59,
                60,
                61
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
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
          "name": "eth1/19/2",
          "controllingPort": 58,
          "pins": [
            {
              "a": {
                "chip": "FC15",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
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
          "name": "eth1/19/3",
          "controllingPort": 58,
          "pins": [
            {
              "a": {
                "chip": "FC15",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                61
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                61
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                61
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                61
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                61
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
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
          "name": "eth1/19/4",
          "controllingPort": 58,
          "pins": [
            {
              "a": {
                "chip": "FC15",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
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
          "name": "eth1/20/1",
          "controllingPort": 62,
          "pins": [
            {
              "a": {
                "chip": "FC14",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/20",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                63
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                63,
                64,
                65
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                63,
                64,
                65
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                63
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                63
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                63,
                64,
                65
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                63,
                64,
                65
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                63
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                63
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                63,
                64,
                65
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
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
          "name": "eth1/20/2",
          "controllingPort": 62,
          "pins": [
            {
              "a": {
                "chip": "FC14",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/20",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
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
          "name": "eth1/20/3",
          "controllingPort": 62,
          "pins": [
            {
              "a": {
                "chip": "FC14",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/20",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                65
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                65
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                65
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                65
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                65
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
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
          "name": "eth1/20/4",
          "controllingPort": 62,
          "pins": [
            {
              "a": {
                "chip": "FC14",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/20",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
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
          "name": "eth1/21/1",
          "controllingPort": 68,
          "pins": [
            {
              "a": {
                "chip": "FC17",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                69
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                69,
                70,
                71
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                69,
                70,
                71
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                69
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                69
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                69,
                70,
                71
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                69,
                70,
                71
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                69
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                69
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                69,
                70,
                71
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
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
          "name": "eth1/21/2",
          "controllingPort": 68,
          "pins": [
            {
              "a": {
                "chip": "FC17",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
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
          "name": "eth1/21/3",
          "controllingPort": 68,
          "pins": [
            {
              "a": {
                "chip": "FC17",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                71
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                71
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                71
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                71
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                71
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
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
          "name": "eth1/21/4",
          "controllingPort": 68,
          "pins": [
            {
              "a": {
                "chip": "FC17",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
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
          "name": "eth1/22/1",
          "controllingPort": 72,
          "pins": [
            {
              "a": {
                "chip": "FC16",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                73
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                73,
                74,
                75
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                73,
                74,
                75
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                73
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                73
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                73,
                74,
                75
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                73,
                74,
                75
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                73
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                73
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                73,
                74,
                75
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
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
          "name": "eth1/22/2",
          "controllingPort": 72,
          "pins": [
            {
              "a": {
                "chip": "FC16",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
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
          "name": "eth1/22/3",
          "controllingPort": 72,
          "pins": [
            {
              "a": {
                "chip": "FC16",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                75
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                75
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                75
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                75
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                75
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
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
          "name": "eth1/22/4",
          "controllingPort": 72,
          "pins": [
            {
              "a": {
                "chip": "FC16",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
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
          "name": "eth1/23/1",
          "controllingPort": 76,
          "pins": [
            {
              "a": {
                "chip": "FC19",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                77
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                77,
                78,
                79
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                77,
                78,
                79
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                77
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                77
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                77,
                78,
                79
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                77,
                78,
                79
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                77
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                77
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                77,
                78,
                79
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
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
          "name": "eth1/23/2",
          "controllingPort": 76,
          "pins": [
            {
              "a": {
                "chip": "FC19",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
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
          "name": "eth1/23/3",
          "controllingPort": 76,
          "pins": [
            {
              "a": {
                "chip": "FC19",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                79
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                79
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                79
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                79
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                79
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
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
          "name": "eth1/23/4",
          "controllingPort": 76,
          "pins": [
            {
              "a": {
                "chip": "FC19",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
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
          "name": "eth1/24/1",
          "controllingPort": 80,
          "pins": [
            {
              "a": {
                "chip": "FC18",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/24",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                81
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                81,
                82,
                83
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                81,
                82,
                83
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                81
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                81
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                81,
                82,
                83
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                81,
                82,
                83
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                81
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                81
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                81,
                82,
                83
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
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
          "name": "eth1/24/2",
          "controllingPort": 80,
          "pins": [
            {
              "a": {
                "chip": "FC18",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/24",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
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
          "name": "eth1/24/3",
          "controllingPort": 80,
          "pins": [
            {
              "a": {
                "chip": "FC18",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/24",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                83
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                83
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                83
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                83
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                83
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
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
          "name": "eth1/24/4",
          "controllingPort": 80,
          "pins": [
            {
              "a": {
                "chip": "FC18",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/24",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
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
          "name": "eth1/25/1",
          "controllingPort": 84,
          "pins": [
            {
              "a": {
                "chip": "FC21",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                85
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                85,
                86,
                87
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                85,
                86,
                87
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                85
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                85
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                85,
                86,
                87
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                85,
                86,
                87
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 72,
                      "post": 38,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 72,
                      "post": 38,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 72,
                      "post": 38,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 72,
                      "post": 38,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                85
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                85
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
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
          "name": "eth1/25/2",
          "controllingPort": 84,
          "pins": [
            {
              "a": {
                "chip": "FC21",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
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
          "name": "eth1/25/3",
          "controllingPort": 84,
          "pins": [
            {
              "a": {
                "chip": "FC21",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                87
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                87
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                87
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                87
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                87
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
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
          "name": "eth1/25/4",
          "controllingPort": 84,
          "pins": [
            {
              "a": {
                "chip": "FC21",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
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
          "name": "eth1/26/1",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "FC20",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                89
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                89,
                90,
                91
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                89,
                90,
                91
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                89
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                89
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                89,
                90,
                91
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                89,
                90,
                91
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 66,
                      "post": 44,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 66,
                      "post": 44,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 66,
                      "post": 44,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 66,
                      "post": 44,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                89
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                89
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
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
          "name": "eth1/26/2",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "FC20",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
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
          "name": "eth1/26/3",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "FC20",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                91
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                91
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                91
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                91
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                91
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
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
          "name": "eth1/26/4",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "FC20",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
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
          "name": "eth1/27/1",
          "controllingPort": 92,
          "pins": [
            {
              "a": {
                "chip": "FC23",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                93
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                93,
                94,
                95
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                93,
                94,
                95
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                93
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                93
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                93,
                94,
                95
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                93,
                94,
                95
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 4,
                      "pre2": 0,
                      "main": 68,
                      "post": 40,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 4,
                      "pre2": 0,
                      "main": 68,
                      "post": 40,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 4,
                      "pre2": 0,
                      "main": 68,
                      "post": 40,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 4,
                      "pre2": 0,
                      "main": 68,
                      "post": 40,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                93
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                93
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
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
          "name": "eth1/27/2",
          "controllingPort": 92,
          "pins": [
            {
              "a": {
                "chip": "FC23",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
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
          "name": "eth1/27/3",
          "controllingPort": 92,
          "pins": [
            {
              "a": {
                "chip": "FC23",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                95
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                95
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                95
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                95
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                95
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
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
          "name": "eth1/27/4",
          "controllingPort": 92,
          "pins": [
            {
              "a": {
                "chip": "FC23",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
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
          "name": "eth1/28/1",
          "controllingPort": 96,
          "pins": [
            {
              "a": {
                "chip": "FC22",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/28",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                97
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                97,
                98,
                99
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                97,
                98,
                99
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                97
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                97
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                97,
                98,
                99
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                97,
                98,
                99
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 68,
                      "post": 42,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 68,
                      "post": 42,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 68,
                      "post": 42,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 68,
                      "post": 42,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                97
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                97
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
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
          "name": "eth1/28/2",
          "controllingPort": 96,
          "pins": [
            {
              "a": {
                "chip": "FC22",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/28",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
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
          "name": "eth1/28/3",
          "controllingPort": 96,
          "pins": [
            {
              "a": {
                "chip": "FC22",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/28",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                99
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                99
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                99
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                99
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                99
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
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
          "name": "eth1/28/4",
          "controllingPort": 96,
          "pins": [
            {
              "a": {
                "chip": "FC22",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/28",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
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
          "name": "eth1/29/1",
          "controllingPort": 102,
          "pins": [
            {
              "a": {
                "chip": "FC25",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                103
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                103,
                104,
                105
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                103,
                104,
                105
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                103
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                103
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                103,
                104,
                105
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                103,
                104,
                105
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 4,
                      "pre2": 0,
                      "main": 64,
                      "post": 44,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 4,
                      "pre2": 0,
                      "main": 64,
                      "post": 44,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 4,
                      "pre2": 0,
                      "main": 64,
                      "post": 44,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 4,
                      "pre2": 0,
                      "main": 64,
                      "post": 44,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                103
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                103
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
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
          "name": "eth1/29/2",
          "controllingPort": 102,
          "pins": [
            {
              "a": {
                "chip": "FC25",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 1
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
          "name": "eth1/29/3",
          "controllingPort": 102,
          "pins": [
            {
              "a": {
                "chip": "FC25",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                105
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                105
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                105
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                105
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                105
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
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
          "name": "eth1/29/4",
          "controllingPort": 102,
          "pins": [
            {
              "a": {
                "chip": "FC25",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 3
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
          "name": "eth1/30/1",
          "controllingPort": 106,
          "pins": [
            {
              "a": {
                "chip": "FC24",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                107
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                107,
                108,
                109
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                107,
                108,
                109
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                107
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                107
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                107,
                108,
                109
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                107,
                108,
                109
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 64,
                      "post": 46,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 64,
                      "post": 46,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 64,
                      "post": 46,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 64,
                      "post": 46,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                107
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                107
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
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
          "name": "eth1/30/2",
          "controllingPort": 106,
          "pins": [
            {
              "a": {
                "chip": "FC24",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
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
          "name": "eth1/30/3",
          "controllingPort": 106,
          "pins": [
            {
              "a": {
                "chip": "FC24",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                109
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                109
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                109
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                109
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                109
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
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
          "name": "eth1/30/4",
          "controllingPort": 106,
          "pins": [
            {
              "a": {
                "chip": "FC24",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
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
          "name": "eth1/31/1",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "FC27",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                111
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                111,
                112,
                113
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                111,
                112,
                113
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                111
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                111
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                111,
                112,
                113
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                111,
                112,
                113
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 62,
                      "post": 48,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 62,
                      "post": 48,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 62,
                      "post": 48,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 62,
                      "post": 48,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                111
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                111
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
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
          "name": "eth1/31/2",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "FC27",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 1
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
          "name": "eth1/31/3",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "FC27",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                113
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                113
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                113
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                113
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                113
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
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
          "name": "eth1/31/4",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "FC27",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 3
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
          "name": "eth1/32/1",
          "controllingPort": 114,
          "pins": [
            {
              "a": {
                "chip": "FC26",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                115
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                115,
                116,
                117
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                115,
                116,
                117
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                115
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                115
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                115,
                116,
                117
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                115,
                116,
                117
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 62,
                      "post": 48,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 62,
                      "post": 48,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 62,
                      "post": 48,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    },
                    "tx": {
                      "pre": 2,
                      "pre2": 0,
                      "main": 62,
                      "post": 48,
                      "post2": 0,
                      "post3": 0,
                      "driveCurrent": 8
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                115
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                115
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
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
          "name": "eth1/32/2",
          "controllingPort": 114,
          "pins": [
            {
              "a": {
                "chip": "FC26",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
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
          "name": "eth1/32/3",
          "controllingPort": 114,
          "pins": [
            {
              "a": {
                "chip": "FC26",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                117
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                117
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                117
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                117
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                117
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
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
          "name": "eth1/32/4",
          "controllingPort": 114,
          "pins": [
            {
              "a": {
                "chip": "FC26",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
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
          "name": "eth1/1/1",
          "controllingPort": 118,
          "pins": [
            {
              "a": {
                "chip": "FC29",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                119
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                119,
                120,
                121
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                119,
                120,
                121
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                119
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                119
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                119,
                120,
                121
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                119,
                120,
                121
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                119
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                119
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                119,
                120,
                121
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
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
          "name": "eth1/1/2",
          "controllingPort": 118,
          "pins": [
            {
              "a": {
                "chip": "FC29",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
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
          "name": "eth1/1/3",
          "controllingPort": 118,
          "pins": [
            {
              "a": {
                "chip": "FC29",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                121
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                121
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                121
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                121
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                121
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
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
          "name": "eth1/1/4",
          "controllingPort": 118,
          "pins": [
            {
              "a": {
                "chip": "FC29",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
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
          "name": "eth1/2/1",
          "controllingPort": 122,
          "pins": [
            {
              "a": {
                "chip": "FC28",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                123
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                123,
                124,
                125
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                123,
                124,
                125
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                123
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                123
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                123,
                124,
                125
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                123,
                124,
                125
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                123
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                123
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                123,
                124,
                125
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
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
          "name": "eth1/2/2",
          "controllingPort": 122,
          "pins": [
            {
              "a": {
                "chip": "FC28",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
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
          "name": "eth1/2/3",
          "controllingPort": 122,
          "pins": [
            {
              "a": {
                "chip": "FC28",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                125
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                125
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                125
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                125
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                125
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
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
          "name": "eth1/2/4",
          "controllingPort": 122,
          "pins": [
            {
              "a": {
                "chip": "FC28",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
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
          "name": "eth1/3/1",
          "controllingPort": 126,
          "pins": [
            {
              "a": {
                "chip": "FC31",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                127
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                127,
                128,
                129
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                127,
                128,
                129
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                127
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                127
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                127,
                128,
                129
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                127,
                128,
                129
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                127
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                127
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                127,
                128,
                129
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
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
          "name": "eth1/3/2",
          "controllingPort": 126,
          "pins": [
            {
              "a": {
                "chip": "FC31",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
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
          "name": "eth1/3/3",
          "controllingPort": 126,
          "pins": [
            {
              "a": {
                "chip": "FC31",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                129
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                129
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                129
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                129
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                129
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
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
          "name": "eth1/3/4",
          "controllingPort": 126,
          "pins": [
            {
              "a": {
                "chip": "FC31",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
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
          "name": "eth1/4/1",
          "controllingPort": 130,
          "pins": [
            {
              "a": {
                "chip": "FC30",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                131
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                131,
                132,
                133
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                131,
                132,
                133
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                131
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                131
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "27": {
              "subsumedPorts": [
                131,
                132,
                133
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                131,
                132,
                133
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                131
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                131
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "33": {
              "subsumedPorts": [
                131,
                132,
                133
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
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
          "name": "eth1/4/2",
          "controllingPort": 130,
          "pins": [
            {
              "a": {
                "chip": "FC30",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
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
          "name": "eth1/4/3",
          "controllingPort": 130,
          "pins": [
            {
              "a": {
                "chip": "FC30",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                133
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                133
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "20": {
              "subsumedPorts": [
                133
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                133
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                133
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
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
          "name": "eth1/4/4",
          "controllingPort": 130,
          "pins": [
            {
              "a": {
                "chip": "FC30",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "15": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "34": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
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
      "name": "FC29",
      "type": 1,
      "physicalID": 29
    },
    {
      "name": "FC28",
      "type": 1,
      "physicalID": 28
    },
    {
      "name": "FC31",
      "type": 1,
      "physicalID": 31
    },
    {
      "name": "FC30",
      "type": 1,
      "physicalID": 30
    },
    {
      "name": "FC1",
      "type": 1,
      "physicalID": 1
    },
    {
      "name": "FC0",
      "type": 1,
      "physicalID": 0
    },
    {
      "name": "FC3",
      "type": 1,
      "physicalID": 3
    },
    {
      "name": "FC2",
      "type": 1,
      "physicalID": 2
    },
    {
      "name": "FC5",
      "type": 1,
      "physicalID": 5
    },
    {
      "name": "FC4",
      "type": 1,
      "physicalID": 4
    },
    {
      "name": "FC7",
      "type": 1,
      "physicalID": 7
    },
    {
      "name": "FC6",
      "type": 1,
      "physicalID": 6
    },
    {
      "name": "FC9",
      "type": 1,
      "physicalID": 9
    },
    {
      "name": "FC8",
      "type": 1,
      "physicalID": 8
    },
    {
      "name": "FC11",
      "type": 1,
      "physicalID": 11
    },
    {
      "name": "FC10",
      "type": 1,
      "physicalID": 10
    },
    {
      "name": "FC13",
      "type": 1,
      "physicalID": 13
    },
    {
      "name": "FC12",
      "type": 1,
      "physicalID": 12
    },
    {
      "name": "FC15",
      "type": 1,
      "physicalID": 15
    },
    {
      "name": "FC14",
      "type": 1,
      "physicalID": 14
    },
    {
      "name": "FC17",
      "type": 1,
      "physicalID": 17
    },
    {
      "name": "FC16",
      "type": 1,
      "physicalID": 16
    },
    {
      "name": "FC19",
      "type": 1,
      "physicalID": 19
    },
    {
      "name": "FC18",
      "type": 1,
      "physicalID": 18
    },
    {
      "name": "FC21",
      "type": 1,
      "physicalID": 21
    },
    {
      "name": "FC20",
      "type": 1,
      "physicalID": 20
    },
    {
      "name": "FC23",
      "type": 1,
      "physicalID": 23
    },
    {
      "name": "FC22",
      "type": 1,
      "physicalID": 22
    },
    {
      "name": "FC25",
      "type": 1,
      "physicalID": 25
    },
    {
      "name": "FC24",
      "type": 1,
      "physicalID": 24
    },
    {
      "name": "FC27",
      "type": 1,
      "physicalID": 27
    },
    {
      "name": "FC26",
      "type": 1,
      "physicalID": 26
    },
    {
      "name": "eth1/1",
      "type": 3,
      "physicalID": 0
    },
    {
      "name": "eth1/2",
      "type": 3,
      "physicalID": 1
    },
    {
      "name": "eth1/3",
      "type": 3,
      "physicalID": 2
    },
    {
      "name": "eth1/4",
      "type": 3,
      "physicalID": 3
    },
    {
      "name": "eth1/5",
      "type": 3,
      "physicalID": 4
    },
    {
      "name": "eth1/6",
      "type": 3,
      "physicalID": 5
    },
    {
      "name": "eth1/7",
      "type": 3,
      "physicalID": 6
    },
    {
      "name": "eth1/8",
      "type": 3,
      "physicalID": 7
    },
    {
      "name": "eth1/9",
      "type": 3,
      "physicalID": 8
    },
    {
      "name": "eth1/10",
      "type": 3,
      "physicalID": 9
    },
    {
      "name": "eth1/11",
      "type": 3,
      "physicalID": 10
    },
    {
      "name": "eth1/12",
      "type": 3,
      "physicalID": 11
    },
    {
      "name": "eth1/13",
      "type": 3,
      "physicalID": 12
    },
    {
      "name": "eth1/14",
      "type": 3,
      "physicalID": 13
    },
    {
      "name": "eth1/15",
      "type": 3,
      "physicalID": 14
    },
    {
      "name": "eth1/16",
      "type": 3,
      "physicalID": 15
    },
    {
      "name": "eth1/17",
      "type": 3,
      "physicalID": 16
    },
    {
      "name": "eth1/18",
      "type": 3,
      "physicalID": 17
    },
    {
      "name": "eth1/19",
      "type": 3,
      "physicalID": 18
    },
    {
      "name": "eth1/20",
      "type": 3,
      "physicalID": 19
    },
    {
      "name": "eth1/21",
      "type": 3,
      "physicalID": 20
    },
    {
      "name": "eth1/22",
      "type": 3,
      "physicalID": 21
    },
    {
      "name": "eth1/23",
      "type": 3,
      "physicalID": 22
    },
    {
      "name": "eth1/24",
      "type": 3,
      "physicalID": 23
    },
    {
      "name": "eth1/25",
      "type": 3,
      "physicalID": 24
    },
    {
      "name": "eth1/26",
      "type": 3,
      "physicalID": 25
    },
    {
      "name": "eth1/27",
      "type": 3,
      "physicalID": 26
    },
    {
      "name": "eth1/28",
      "type": 3,
      "physicalID": 27
    },
    {
      "name": "eth1/29",
      "type": 3,
      "physicalID": 28
    },
    {
      "name": "eth1/30",
      "type": 3,
      "physicalID": 29
    },
    {
      "name": "eth1/31",
      "type": 3,
      "physicalID": 30
    },
    {
      "name": "eth1/32",
      "type": 3,
      "physicalID": 31
    }
  ],
  "portConfigOverrides": [
    {
      "factor": {
        "ports": [
          122,
          123,
          124,
          125
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          1.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 4,
              "pre2": 0,
              "main": 60,
              "post": 48,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 10
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          122,
          123,
          124,
          125
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          1.5
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 4,
              "pre2": 0,
              "main": 60,
              "post": 48,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 10
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          122,
          123,
          124,
          125
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          2.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 4,
              "pre2": 0,
              "main": 60,
              "post": 48,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 10
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          122,
          123,
          124,
          125
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          2.5
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 6,
              "pre2": 0,
              "main": 62,
              "post": 50,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 12
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          122,
          123,
          124,
          125
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          3.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 6,
              "pre2": 0,
              "main": 62,
              "post": 50,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 12
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          118,
          119,
          120,
          121,
          126,
          127,
          128,
          129,
          130,
          131,
          132,
          133
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          1.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 6,
              "pre2": 0,
              "main": 64,
              "post": 42,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 10
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          118,
          119,
          120,
          121,
          126,
          127,
          128,
          129,
          130,
          131,
          132,
          133
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          1.5
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 7,
              "pre2": 0,
              "main": 62,
              "post": 43,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 10
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          118,
          119,
          120,
          121,
          126,
          127,
          128,
          129,
          130,
          131,
          132,
          133
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          2.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 8,
              "pre2": 0,
              "main": 60,
              "post": 44,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 11
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          118,
          119,
          120,
          121,
          126,
          127,
          128,
          129,
          130,
          131,
          132,
          133
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          2.5
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 7,
              "pre2": 0,
              "main": 61,
              "post": 44,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 12
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          118,
          119,
          120,
          121,
          126,
          127,
          128,
          129,
          130,
          131,
          132,
          133
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          3.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 6,
              "pre2": 0,
              "main": 60,
              "post": 46,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 12
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          1,
          2,
          3,
          4,
          5,
          6,
          7,
          8,
          92,
          93,
          94,
          95,
          96,
          97,
          98,
          99
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          1.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 8,
              "pre2": 0,
              "main": 66,
              "post": 38,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 9
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          1,
          2,
          3,
          4,
          5,
          6,
          7,
          8,
          92,
          93,
          94,
          95,
          96,
          97,
          98,
          99
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          1.5
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 9,
              "pre2": 0,
              "main": 65,
              "post": 38,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 9
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          1,
          2,
          3,
          4,
          5,
          6,
          7,
          8,
          92,
          93,
          94,
          95,
          96,
          97,
          98,
          99
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          2.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 9,
              "pre2": 0,
              "main": 64,
              "post": 39,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 9
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          1,
          2,
          3,
          4,
          5,
          6,
          7,
          8,
          92,
          93,
          94,
          95,
          96,
          97,
          98,
          99
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          2.5
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 9,
              "pre2": 0,
              "main": 63,
              "post": 40,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 9
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          1,
          2,
          3,
          4,
          5,
          6,
          7,
          8,
          92,
          93,
          94,
          95,
          96,
          97,
          98,
          99
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          3.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 8,
              "pre2": 0,
              "main": 64,
              "post": 40,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 10
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          9,
          10,
          11,
          12,
          13,
          14,
          15,
          16,
          76,
          77,
          78,
          79,
          80,
          81,
          82,
          83,
          84,
          85,
          86,
          87,
          88,
          89,
          90,
          91
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          1.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 6,
              "pre2": 0,
              "main": 70,
              "post": 36,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 8
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          9,
          10,
          11,
          12,
          13,
          14,
          15,
          16,
          76,
          77,
          78,
          79,
          80,
          81,
          82,
          83,
          84,
          85,
          86,
          87,
          88,
          89,
          90,
          91
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          1.5
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 6,
              "pre2": 0,
              "main": 70,
              "post": 36,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 9
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          9,
          10,
          11,
          12,
          13,
          14,
          15,
          16,
          76,
          77,
          78,
          79,
          80,
          81,
          82,
          83,
          84,
          85,
          86,
          87,
          88,
          89,
          90,
          91
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          2.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 7,
              "pre2": 0,
              "main": 69,
              "post": 36,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 9
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          9,
          10,
          11,
          12,
          13,
          14,
          15,
          16,
          76,
          77,
          78,
          79,
          80,
          81,
          82,
          83,
          84,
          85,
          86,
          87,
          88,
          89,
          90,
          91
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          2.5
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 8,
              "pre2": 0,
              "main": 67,
              "post": 37,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 9
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          9,
          10,
          11,
          12,
          13,
          14,
          15,
          16,
          76,
          77,
          78,
          79,
          80,
          81,
          82,
          83,
          84,
          85,
          86,
          87,
          88,
          89,
          90,
          91
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          3.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 8,
              "pre2": 0,
              "main": 67,
              "post": 37,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 10
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          17,
          18,
          19,
          20,
          21,
          22,
          23,
          24,
          25,
          26,
          27,
          28,
          29,
          30,
          31,
          32,
          68,
          69,
          70,
          71,
          72,
          73,
          74,
          75
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          1.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 6,
              "pre2": 0,
              "main": 76,
              "post": 30,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 8
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          17,
          18,
          19,
          20,
          21,
          22,
          23,
          24,
          25,
          26,
          27,
          28,
          29,
          30,
          31,
          32,
          68,
          69,
          70,
          71,
          72,
          73,
          74,
          75
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          1.5
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 7,
              "pre2": 0,
              "main": 75,
              "post": 30,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 9
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          17,
          18,
          19,
          20,
          21,
          22,
          23,
          24,
          25,
          26,
          27,
          28,
          29,
          30,
          31,
          32,
          68,
          69,
          70,
          71,
          72,
          73,
          74,
          75
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          2.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 7,
              "pre2": 0,
              "main": 75,
              "post": 30,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 9
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          17,
          18,
          19,
          20,
          21,
          22,
          23,
          24,
          25,
          26,
          27,
          28,
          29,
          30,
          31,
          32,
          68,
          69,
          70,
          71,
          72,
          73,
          74,
          75
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          2.5
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 8,
              "pre2": 0,
              "main": 73,
              "post": 31,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 9
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          17,
          18,
          19,
          20,
          21,
          22,
          23,
          24,
          25,
          26,
          27,
          28,
          29,
          30,
          31,
          32,
          68,
          69,
          70,
          71,
          72,
          73,
          74,
          75
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          3.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 8,
              "pre2": 0,
              "main": 72,
              "post": 32,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 10
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          34,
          35,
          36,
          37,
          38,
          39,
          40,
          41,
          54,
          55,
          56,
          57,
          58,
          59,
          60,
          61,
          62,
          63,
          64,
          65
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          1.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 6,
              "pre2": 0,
              "main": 78,
              "post": 28,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 8
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          34,
          35,
          36,
          37,
          38,
          39,
          40,
          41,
          54,
          55,
          56,
          57,
          58,
          59,
          60,
          61,
          62,
          63,
          64,
          65
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          1.5
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 6,
              "pre2": 0,
              "main": 77,
              "post": 29,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 9
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          34,
          35,
          36,
          37,
          38,
          39,
          40,
          41,
          54,
          55,
          56,
          57,
          58,
          59,
          60,
          61,
          62,
          63,
          64,
          65
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          2.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 7,
              "pre2": 0,
              "main": 75,
              "post": 30,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 10
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          34,
          35,
          36,
          37,
          38,
          39,
          40,
          41,
          54,
          55,
          56,
          57,
          58,
          59,
          60,
          61,
          62,
          63,
          64,
          65
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          2.5
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 8,
              "pre2": 0,
              "main": 73,
              "post": 31,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 10
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          34,
          35,
          36,
          37,
          38,
          39,
          40,
          41,
          54,
          55,
          56,
          57,
          58,
          59,
          60,
          61,
          62,
          63,
          64,
          65
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          3.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 8,
              "pre2": 0,
              "main": 72,
              "post": 32,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 10
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          42,
          43,
          44,
          45,
          46,
          47,
          48,
          49,
          50,
          51,
          52,
          53
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          1.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 6,
              "pre2": 0,
              "main": 80,
              "post": 26,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 8
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          42,
          43,
          44,
          45,
          46,
          47,
          48,
          49,
          50,
          51,
          52,
          53
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          1.5
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 6,
              "pre2": 0,
              "main": 78,
              "post": 28,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 9
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          42,
          43,
          44,
          45,
          46,
          47,
          48,
          49,
          50,
          51,
          52,
          53
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          2.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 6,
              "pre2": 0,
              "main": 78,
              "post": 28,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 9
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          42,
          43,
          44,
          45,
          46,
          47,
          48,
          49,
          50,
          51,
          52,
          53
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          2.5
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 7,
              "pre2": 0,
              "main": 75,
              "post": 30,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 9
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          42,
          43,
          44,
          45,
          46,
          47,
          48,
          49,
          50,
          51,
          52,
          53
        ],
        "profiles": [
          11,
          13,
          14,
          15,
          17,
          19,
          20,
          27
        ],
        "cableLengths": [
          3.0
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 8,
              "pre2": 0,
              "main": 74,
              "post": 30,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 9
            }
          }
        ]
      }
    },
    {
      "factor": {
        "ports": [
          118,
          119,
          120,
          121,
          122,
          123,
          124,
          125,
          126,
          127,
          128,
          129,
          130,
          131,
          132,
          133,
          1,
          2,
          3,
          4,
          5,
          6,
          7,
          8,
          9,
          10,
          11,
          12,
          13,
          14,
          15,
          16,
          17,
          18,
          19,
          20,
          21,
          22,
          23,
          24,
          25,
          26,
          27,
          28,
          29,
          30,
          31,
          32,
          34,
          35,
          36,
          37,
          38,
          39,
          40,
          41,
          42,
          43,
          44,
          45,
          46,
          47,
          48,
          49,
          50,
          51,
          52,
          53,
          54,
          55,
          56,
          57,
          58,
          59,
          60,
          61,
          62,
          63,
          64,
          65,
          68,
          69,
          70,
          71,
          72,
          73,
          74,
          75,
          76,
          77,
          78,
          79,
          80,
          81,
          82,
          83
        ],
        "profiles": [
          33,
          34
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": 16,
              "pre2": 0,
              "main": 96,
              "post": 56,
              "post2": 0,
              "post3": 0,
              "driveCurrent": 8
            }
          }
        ]
      }
    }
  ],
  "platformSupportedProfiles": [
    {
      "factor": {
        "profileID": 11
      },
      "profile": {
        "speed": 10000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 10,
          "interfaceType": 10
        }
      }
    },
    {
      "factor": {
        "profileID": 12
      },
      "profile": {
        "speed": 10000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "fec": 1,
          "medium": 2,
          "interfaceMode": 41,
          "interfaceType": 41
        }
      }
    },
    {
      "factor": {
        "profileID": 13
      },
      "profile": {
        "speed": 20000,
        "iphy": {
          "numLanes": 2,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 11,
          "interfaceType": 11
        }
      }
    },
    {
      "factor": {
        "profileID": 29
      },
      "profile": {
        "speed": 20000,
        "iphy": {
          "numLanes": 2,
          "modulation": 1,
          "fec": 1,
          "medium": 2,
          "interfaceMode": 40,
          "interfaceType": 40
        }
      }
    },
    {
      "factor": {
        "profileID": 14
      },
      "profile": {
        "speed": 25000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 10,
          "interfaceType": 10
        }
      }
    },
    {
      "factor": {
        "profileID": 15
      },
      "profile": {
        "speed": 25000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "fec": 74,
          "medium": 1,
          "interfaceMode": 10,
          "interfaceType": 10
        }
      }
    },
    {
      "factor": {
        "profileID": 30
      },
      "profile": {
        "speed": 25000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "fec": 1,
          "medium": 2,
          "interfaceMode": 30,
          "interfaceType": 30
        }
      }
    },
    {
      "factor": {
        "profileID": 17
      },
      "profile": {
        "speed": 40000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 12,
          "interfaceType": 12
        }
      }
    },
    {
      "factor": {
        "profileID": 18
      },
      "profile": {
        "speed": 40000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 1,
          "medium": 2,
          "interfaceMode": 40,
          "interfaceType": 40
        }
      }
    },
    {
      "factor": {
        "profileID": 19
      },
      "profile": {
        "speed": 50000,
        "iphy": {
          "numLanes": 2,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 11,
          "interfaceType": 11
        }
      }
    },
    {
      "factor": {
        "profileID": 20
      },
      "profile": {
        "speed": 50000,
        "iphy": {
          "numLanes": 2,
          "modulation": 1,
          "fec": 74,
          "medium": 1,
          "interfaceMode": 11,
          "interfaceType": 11
        }
      }
    },
    {
      "factor": {
        "profileID": 31
      },
      "profile": {
        "speed": 50000,
        "iphy": {
          "numLanes": 2,
          "modulation": 1,
          "fec": 1,
          "medium": 2,
          "interfaceMode": 30,
          "interfaceType": 30
        }
      }
    },
    {
      "factor": {
        "profileID": 27
      },
      "profile": {
        "speed": 100000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 91,
          "medium": 1,
          "interfaceMode": 12,
          "interfaceType": 12
        }
      }
    },
    {
      "factor": {
        "profileID": 28
      },
      "profile": {
        "speed": 100000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 91,
          "medium": 2,
          "interfaceMode": 30,
          "interfaceType": 30
        }
      }
    },
    {
      "factor": {
        "profileID": 33
      },
      "profile": {
        "speed": 100000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 91,
          "medium": 1,
          "interfaceMode": 12,
          "interfaceType": 12
        }
      }
    },
    {
      "factor": {
        "profileID": 34
      },
      "profile": {
        "speed": 25000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 10,
          "interfaceType": 10
        }
      }
    }
  ]
}
)";
} // namespace

namespace facebook {
namespace fboss {
Wedge100PlatformMapping::Wedge100PlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

Wedge100PlatformMapping::Wedge100PlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

void Wedge100PlatformMapping::customizePlatformPortConfigOverrideFactor(
    std::optional<cfg::PlatformPortConfigOverrideFactor>& factor) const {
  // Wedge100PlatformMapping downlink cable length can only support 1.0 to 3.0
  if (factor && factor->cableLengths() && !factor->cableLengths()->empty()) {
    auto cableLength = (*factor->cableLengths())[0];
    factor->cableLengths() = {std::max(1.0, std::min(3.0, cableLength))};
  }
}
} // namespace fboss
} // namespace facebook
