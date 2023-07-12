/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/meru400bia/Meru400biaPlatformMapping.h"

namespace facebook::fboss {
namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{
  "ports": {
    "1": {
      "mapping": {
        "id": 1,
        "name": "rcy1/1/1",
        "controllingPort": 1,
        "pins": [
          {
            "a": {
              "chip": "rcy1",
              "lane": 0
            }
          }
        ],
        "portType": 3,
        "attachedCoreId": 0,
        "attachedCorePortIndex": 1
      },
      "supportedProfiles": {
        "11": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "rcy1",
                  "lane": 0
                }
              }
            ],
            "transceiver": [
              {
                "id": {
                  "chip": "rcy1/1",
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
        "name": "eth1/18/1",
        "controllingPort": 2,
        "pins": [
          {
            "a": {
              "chip": "BC0",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/18",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "BC0",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/18",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "BC0",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/18",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "BC0",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/18",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC1",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/18",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC1",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/18",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC1",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/18",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC1",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/18",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 0,
        "attachedCorePortIndex": 2
      },
      "supportedProfiles": {
        "22": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC0",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC0",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC0",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC0",
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
        "23": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC0",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC0",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC0",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC0",
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
        }
      }
    },
    "3": {
      "mapping": {
        "id": 3,
        "name": "eth1/17/1",
        "controllingPort": 3,
        "pins": [
          {
            "a": {
              "chip": "BC2",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/17",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "BC2",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/17",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "BC2",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/17",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "BC2",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/17",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC3",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/17",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC3",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/17",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC3",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/17",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC3",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/17",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 0,
        "attachedCorePortIndex": 3
      },
      "supportedProfiles": {
        "22": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC2",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC2",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC2",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC2",
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
        "23": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC2",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC2",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC2",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC2",
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
        }
      }
    },
    "4": {
      "mapping": {
        "id": 4,
        "name": "eth1/16/1",
        "controllingPort": 4,
        "pins": [
          {
            "a": {
              "chip": "BC4",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/16",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "BC4",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/16",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "BC4",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/16",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "BC4",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/16",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC5",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/16",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC5",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/16",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC5",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/16",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC5",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/16",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 0,
        "attachedCorePortIndex": 4
      },
      "supportedProfiles": {
        "22": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC4",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC4",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC4",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC4",
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
        "23": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC4",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC4",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC4",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC4",
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
        }
      }
    },
    "5": {
      "mapping": {
        "id": 5,
        "name": "eth1/15/1",
        "controllingPort": 5,
        "pins": [
          {
            "a": {
              "chip": "BC6",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/15",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "BC6",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/15",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "BC6",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/15",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "BC6",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/15",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC7",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/15",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC7",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/15",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC7",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/15",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC7",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/15",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 0,
        "attachedCorePortIndex": 5
      },
      "supportedProfiles": {
        "22": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC6",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC6",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC6",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC6",
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
        "23": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC6",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC6",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC6",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC6",
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
        }
      }
    },
    "6": {
      "mapping": {
        "id": 6,
        "name": "eth1/14/1",
        "controllingPort": 6,
        "pins": [
          {
            "a": {
              "chip": "BC8",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/14",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "BC8",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/14",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "BC8",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/14",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "BC8",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/14",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC9",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/14",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC9",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/14",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC9",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/14",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC9",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/14",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 0,
        "attachedCorePortIndex": 6
      },
      "supportedProfiles": {
        "22": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC8",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC8",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC8",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC8",
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
        "23": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC8",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC8",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC8",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC8",
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
        }
      }
    },
    "7": {
      "mapping": {
        "id": 7,
        "name": "eth1/13/1",
        "controllingPort": 7,
        "pins": [
          {
            "a": {
              "chip": "BC10",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/13",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "BC10",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/13",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "BC10",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/13",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "BC10",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/13",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC11",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/13",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC11",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/13",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC11",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/13",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC11",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/13",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 0,
        "attachedCorePortIndex": 7
      },
      "supportedProfiles": {
        "22": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC10",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC10",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC10",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC10",
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
        "23": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC10",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC10",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC10",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC10",
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
        }
      }
    },
    "8": {
      "mapping": {
        "id": 8,
        "name": "eth1/12/1",
        "controllingPort": 8,
        "pins": [
          {
            "a": {
              "chip": "BC12",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/12",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "BC12",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/12",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "BC12",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/12",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "BC12",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/12",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC13",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/12",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC13",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/12",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC13",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/12",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC13",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/12",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 0,
        "attachedCorePortIndex": 8
      },
      "supportedProfiles": {
        "22": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC12",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC12",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC12",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC12",
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
        "23": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC12",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC12",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC12",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC12",
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
        }
      }
    },
    "9": {
      "mapping": {
        "id": 9,
        "name": "eth1/11/1",
        "controllingPort": 9,
        "pins": [
          {
            "a": {
              "chip": "BC14",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/11",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "BC14",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/11",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "BC14",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/11",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "BC14",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/11",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC15",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/11",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC15",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/11",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC15",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/11",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC15",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/11",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 0,
        "attachedCorePortIndex": 9
      },
      "supportedProfiles": {
        "22": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC14",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC14",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC14",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC14",
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
        "23": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC14",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC14",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC14",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC14",
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
        }
      }
    },
    "10": {
      "mapping": {
        "id": 10,
        "name": "eth1/10/1",
        "controllingPort": 10,
        "pins": [
          {
            "a": {
              "chip": "BC16",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/10",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "BC16",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/10",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "BC16",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/10",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "BC16",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/10",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC17",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/10",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC17",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/10",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC17",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/10",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC17",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/10",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 0,
        "attachedCorePortIndex": 10
      },
      "supportedProfiles": {
        "22": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC16",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC16",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC16",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC16",
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
        "23": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC16",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC16",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC16",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC16",
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
        }
      }
    },
    "11": {
      "mapping": {
        "id": 11,
        "name": "eth1/1/1",
        "controllingPort": 11,
        "pins": [
          {
            "a": {
              "chip": "BC18",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/1",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "BC18",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/1",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "BC18",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/1",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "BC18",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/1",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC19",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/1",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC19",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/1",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC19",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/1",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC19",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/1",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 1,
        "attachedCorePortIndex": 11
      },
      "supportedProfiles": {
        "22": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC18",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC18",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC18",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC18",
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
        "23": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC18",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC18",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC18",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC18",
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
        }
      }
    },
    "12": {
      "mapping": {
        "id": 12,
        "name": "eth1/2/1",
        "controllingPort": 12,
        "pins": [
          {
            "a": {
              "chip": "BC20",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/2",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "BC20",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/2",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "BC20",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/2",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "BC20",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/2",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC21",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/2",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC21",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/2",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC21",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/2",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC21",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/2",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 1,
        "attachedCorePortIndex": 12
      },
      "supportedProfiles": {
        "22": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC20",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC20",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC20",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC20",
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
        "23": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC20",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC20",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC20",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC20",
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
        }
      }
    },
    "13": {
      "mapping": {
        "id": 13,
        "name": "eth1/3/1",
        "controllingPort": 13,
        "pins": [
          {
            "a": {
              "chip": "BC22",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/3",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "BC22",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/3",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "BC22",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/3",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "BC22",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/3",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC23",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/3",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC23",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/3",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC23",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/3",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC23",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/3",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 1,
        "attachedCorePortIndex": 13
      },
      "supportedProfiles": {
        "22": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC22",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC22",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC22",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC22",
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
        "23": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC22",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC22",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC22",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC22",
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
        }
      }
    },
    "14": {
      "mapping": {
        "id": 14,
        "name": "eth1/4/1",
        "controllingPort": 14,
        "pins": [
          {
            "a": {
              "chip": "BC24",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/4",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "BC24",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/4",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "BC24",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/4",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "BC24",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/4",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC25",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/4",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC25",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/4",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC25",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/4",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC25",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/4",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 1,
        "attachedCorePortIndex": 14
      },
      "supportedProfiles": {
        "22": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC24",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC24",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC24",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC24",
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
        "23": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC24",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC24",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC24",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC24",
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
        }
      }
    },
    "15": {
      "mapping": {
        "id": 15,
        "name": "eth1/5/1",
        "controllingPort": 15,
        "pins": [
          {
            "a": {
              "chip": "BC26",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/5",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "BC26",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/5",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "BC26",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/5",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "BC26",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/5",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC27",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/5",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC27",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/5",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC27",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/5",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC27",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/5",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 1,
        "attachedCorePortIndex": 15
      },
      "supportedProfiles": {
        "22": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC26",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC26",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC26",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC26",
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
        "23": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC26",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC26",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC26",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC26",
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
        }
      }
    },
    "16": {
      "mapping": {
        "id": 16,
        "name": "eth1/6/1",
        "controllingPort": 16,
        "pins": [
          {
            "a": {
              "chip": "BC28",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/6",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "BC28",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/6",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "BC28",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/6",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "BC28",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/6",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC29",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/6",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC29",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/6",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC29",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/6",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC29",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/6",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 1,
        "attachedCorePortIndex": 16
      },
      "supportedProfiles": {
        "22": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC28",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC28",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC28",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC28",
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
        "23": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC28",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC28",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC28",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC28",
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
        }
      }
    },
    "17": {
      "mapping": {
        "id": 17,
        "name": "eth1/7/1",
        "controllingPort": 17,
        "pins": [
          {
            "a": {
              "chip": "BC30",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/7",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "BC30",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/7",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "BC30",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/7",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "BC30",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/7",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC31",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/7",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC31",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/7",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC31",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/7",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC31",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/7",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 1,
        "attachedCorePortIndex": 17
      },
      "supportedProfiles": {
        "22": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC30",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC30",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC30",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC30",
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
        "23": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC30",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC30",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC30",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC30",
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
        }
      }
    },
    "18": {
      "mapping": {
        "id": 18,
        "name": "eth1/8/1",
        "controllingPort": 18,
        "pins": [
          {
            "a": {
              "chip": "BC32",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/8",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "BC32",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/8",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "BC32",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/8",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "BC32",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/8",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC33",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/8",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC33",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/8",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC33",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/8",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC33",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/8",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 1,
        "attachedCorePortIndex": 18
      },
      "supportedProfiles": {
        "22": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC32",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC32",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC32",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC32",
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
        "23": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC32",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC32",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC32",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC32",
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
        }
      }
    },
    "19": {
      "mapping": {
        "id": 19,
        "name": "eth1/9/1",
        "controllingPort": 19,
        "pins": [
          {
            "a": {
              "chip": "BC34",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/9",
                "lane": 0
              }
            }
          },
          {
            "a": {
              "chip": "BC34",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/9",
                "lane": 1
              }
            }
          },
          {
            "a": {
              "chip": "BC34",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/9",
                "lane": 2
              }
            }
          },
          {
            "a": {
              "chip": "BC34",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/9",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC35",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/9",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC35",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "eth1/9",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC35",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "eth1/9",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC35",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "eth1/9",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 1,
        "attachedCorePortIndex": 19
      },
      "supportedProfiles": {
        "22": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC34",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC34",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC34",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC34",
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
        "23": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC34",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC34",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC34",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC34",
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
        }
      }
    }
  },
  "chips": [
    {
      "name": "rcy1",
      "type": 1,
      "physicalID": 55
    },
    {
      "name": "BC0",
      "type": 1,
      "physicalID": 0
    },
    {
      "name": "BC1",
      "type": 1,
      "physicalID": 1
    },
    {
      "name": "BC2",
      "type": 1,
      "physicalID": 2
    },
    {
      "name": "BC3",
      "type": 1,
      "physicalID": 3
    },
    {
      "name": "BC4",
      "type": 1,
      "physicalID": 4
    },
    {
      "name": "BC5",
      "type": 1,
      "physicalID": 5
    },
    {
      "name": "BC6",
      "type": 1,
      "physicalID": 6
    },
    {
      "name": "BC7",
      "type": 1,
      "physicalID": 7
    },
    {
      "name": "BC8",
      "type": 1,
      "physicalID": 8
    },
    {
      "name": "BC9",
      "type": 1,
      "physicalID": 9
    },
    {
      "name": "BC10",
      "type": 1,
      "physicalID": 10
    },
    {
      "name": "BC11",
      "type": 1,
      "physicalID": 11
    },
    {
      "name": "BC12",
      "type": 1,
      "physicalID": 12
    },
    {
      "name": "BC13",
      "type": 1,
      "physicalID": 13
    },
    {
      "name": "BC14",
      "type": 1,
      "physicalID": 14
    },
    {
      "name": "BC15",
      "type": 1,
      "physicalID": 15
    },
    {
      "name": "BC16",
      "type": 1,
      "physicalID": 16
    },
    {
      "name": "BC17",
      "type": 1,
      "physicalID": 17
    },
    {
      "name": "BC18",
      "type": 1,
      "physicalID": 18
    },
    {
      "name": "BC19",
      "type": 1,
      "physicalID": 19
    },
    {
      "name": "BC20",
      "type": 1,
      "physicalID": 20
    },
    {
      "name": "BC21",
      "type": 1,
      "physicalID": 21
    },
    {
      "name": "BC22",
      "type": 1,
      "physicalID": 22
    },
    {
      "name": "BC23",
      "type": 1,
      "physicalID": 23
    },
    {
      "name": "BC24",
      "type": 1,
      "physicalID": 24
    },
    {
      "name": "BC25",
      "type": 1,
      "physicalID": 25
    },
    {
      "name": "BC26",
      "type": 1,
      "physicalID": 26
    },
    {
      "name": "BC27",
      "type": 1,
      "physicalID": 27
    },
    {
      "name": "BC28",
      "type": 1,
      "physicalID": 28
    },
    {
      "name": "BC29",
      "type": 1,
      "physicalID": 29
    },
    {
      "name": "BC30",
      "type": 1,
      "physicalID": 30
    },
    {
      "name": "BC31",
      "type": 1,
      "physicalID": 31
    },
    {
      "name": "BC32",
      "type": 1,
      "physicalID": 32
    },
    {
      "name": "BC33",
      "type": 1,
      "physicalID": 33
    },
    {
      "name": "BC34",
      "type": 1,
      "physicalID": 34
    },
    {
      "name": "BC35",
      "type": 1,
      "physicalID": 35
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
    }
  ],
  "platformSettings": {},
  "portConfigOverrides": [],
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
        "profileID": 22
      },
      "profile": {
        "speed": 100000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 528,
          "medium": 1,
          "interfaceMode": 12,
          "interfaceType": 12
        }
      }
    },
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
          "medium": 3,
          "interfaceMode": 12,
          "interfaceType": 12
        }
      }
    },
    {
      "factor": {
        "profileID": 26
      },
      "profile": {
        "speed": 400000,
        "iphy": {
          "numLanes": 8,
          "modulation": 2,
          "fec": 545,
          "medium": 3,
          "interfaceMode": 41,
          "interfaceType": 41
        }
      }
    },
    {
      "factor": {
        "profileID": 35
      },
      "profile": {
        "speed": 400000,
        "iphy": {
          "numLanes": 8,
          "modulation": 2,
          "fec": 545,
          "medium": 1,
          "interfaceMode": 41,
          "interfaceType": 41
        }
      }
    }
  ]
}
)";

} // namespace

Meru400biaPlatformMapping::Meru400biaPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

Meru400biaPlatformMapping::Meru400biaPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace facebook::fboss
