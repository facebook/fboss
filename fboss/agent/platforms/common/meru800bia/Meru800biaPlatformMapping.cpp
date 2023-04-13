/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/meru800bia/Meru800biaPlatformMapping.h"

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
          "subsumedPorts": [],
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "rcy1",
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
        "name": "eth1/19/1",
        "controllingPort": 2,
        "pins": [
          {
            "a": {
              "chip": "BC0",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/19",
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
                "chip": "eth1/19",
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
                "chip": "eth1/19",
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
                "chip": "eth1/19",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC0",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "eth1/19",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC0",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "eth1/19",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC0",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "eth1/19",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC0",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "eth1/19",
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
        "38": {
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
            ]
          }
        },
        "39": {
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
              },
              {
                "id": {
                  "chip": "BC0",
                  "lane": 4
                }
              },
              {
                "id": {
                  "chip": "BC0",
                  "lane": 5
                }
              },
              {
                "id": {
                  "chip": "BC0",
                  "lane": 6
                }
              },
              {
                "id": {
                  "chip": "BC0",
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
        "name": "eth1/16/1",
        "controllingPort": 3,
        "pins": [
          {
            "a": {
              "chip": "BC1",
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
              "chip": "BC1",
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
              "chip": "BC1",
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
              "chip": "BC1",
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
              "chip": "BC1",
              "lane": 4
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
              "chip": "BC1",
              "lane": 5
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
              "chip": "BC1",
              "lane": 6
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
              "chip": "BC1",
              "lane": 7
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
        "attachedCorePortIndex": 3
      },
      "supportedProfiles": {
        "38": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC1",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC1",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC1",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC1",
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
                  "chip": "BC1",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC1",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC1",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC1",
                  "lane": 3
                }
              },
              {
                "id": {
                  "chip": "BC1",
                  "lane": 4
                }
              },
              {
                "id": {
                  "chip": "BC1",
                  "lane": 5
                }
              },
              {
                "id": {
                  "chip": "BC1",
                  "lane": 6
                }
              },
              {
                "id": {
                  "chip": "BC1",
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
        "name": "eth1/18/1",
        "controllingPort": 4,
        "pins": [
          {
            "a": {
              "chip": "BC2",
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
              "chip": "BC2",
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
              "chip": "BC2",
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
              "chip": "BC2",
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
              "chip": "BC2",
              "lane": 4
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
              "chip": "BC2",
              "lane": 5
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
              "chip": "BC2",
              "lane": 6
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
              "chip": "BC2",
              "lane": 7
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
        "attachedCorePortIndex": 4
      },
      "supportedProfiles": {
        "38": {
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
            ]
          }
        },
        "39": {
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
              },
              {
                "id": {
                  "chip": "BC2",
                  "lane": 4
                }
              },
              {
                "id": {
                  "chip": "BC2",
                  "lane": 5
                }
              },
              {
                "id": {
                  "chip": "BC2",
                  "lane": 6
                }
              },
              {
                "id": {
                  "chip": "BC2",
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
        "name": "eth1/15/1",
        "controllingPort": 5,
        "pins": [
          {
            "a": {
              "chip": "BC3",
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
              "chip": "BC3",
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
              "chip": "BC3",
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
              "chip": "BC3",
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
              "chip": "BC3",
              "lane": 4
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
              "chip": "BC3",
              "lane": 5
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
              "chip": "BC3",
              "lane": 6
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
              "chip": "BC3",
              "lane": 7
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
        "38": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC3",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC3",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC3",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC3",
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
                  "chip": "BC3",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC3",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC3",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC3",
                  "lane": 3
                }
              },
              {
                "id": {
                  "chip": "BC3",
                  "lane": 4
                }
              },
              {
                "id": {
                  "chip": "BC3",
                  "lane": 5
                }
              },
              {
                "id": {
                  "chip": "BC3",
                  "lane": 6
                }
              },
              {
                "id": {
                  "chip": "BC3",
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
        "name": "eth1/17/1",
        "controllingPort": 6,
        "pins": [
          {
            "a": {
              "chip": "BC4",
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
              "chip": "BC4",
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
              "chip": "BC4",
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
              "chip": "BC4",
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
              "chip": "BC4",
              "lane": 4
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
              "chip": "BC4",
              "lane": 5
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
              "chip": "BC4",
              "lane": 6
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
              "chip": "BC4",
              "lane": 7
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
        "attachedCorePortIndex": 6
      },
      "supportedProfiles": {
        "38": {
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
            ]
          }
        },
        "39": {
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
              },
              {
                "id": {
                  "chip": "BC4",
                  "lane": 4
                }
              },
              {
                "id": {
                  "chip": "BC4",
                  "lane": 5
                }
              },
              {
                "id": {
                  "chip": "BC4",
                  "lane": 6
                }
              },
              {
                "id": {
                  "chip": "BC4",
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
        "name": "eth1/13/1",
        "controllingPort": 7,
        "pins": [
          {
            "a": {
              "chip": "BC5",
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
              "chip": "BC5",
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
              "chip": "BC5",
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
              "chip": "BC5",
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
              "chip": "BC5",
              "lane": 4
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
              "chip": "BC5",
              "lane": 5
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
              "chip": "BC5",
              "lane": 6
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
              "chip": "BC5",
              "lane": 7
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
        "attachedCoreId": 1,
        "attachedCorePortIndex": 7
      },
      "supportedProfiles": {
        "38": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC5",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC5",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC5",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC5",
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
                  "chip": "BC5",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC5",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC5",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC5",
                  "lane": 3
                }
              },
              {
                "id": {
                  "chip": "BC5",
                  "lane": 4
                }
              },
              {
                "id": {
                  "chip": "BC5",
                  "lane": 5
                }
              },
              {
                "id": {
                  "chip": "BC5",
                  "lane": 6
                }
              },
              {
                "id": {
                  "chip": "BC5",
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
        "name": "eth1/11/1",
        "controllingPort": 8,
        "pins": [
          {
            "a": {
              "chip": "BC6",
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
              "chip": "BC6",
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
              "chip": "BC6",
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
              "chip": "BC6",
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
              "chip": "BC6",
              "lane": 4
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
              "chip": "BC6",
              "lane": 5
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
              "chip": "BC6",
              "lane": 6
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
              "chip": "BC6",
              "lane": 7
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
        "attachedCoreId": 1,
        "attachedCorePortIndex": 8
      },
      "supportedProfiles": {
        "38": {
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
            ]
          }
        },
        "39": {
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
              },
              {
                "id": {
                  "chip": "BC6",
                  "lane": 4
                }
              },
              {
                "id": {
                  "chip": "BC6",
                  "lane": 5
                }
              },
              {
                "id": {
                  "chip": "BC6",
                  "lane": 6
                }
              },
              {
                "id": {
                  "chip": "BC6",
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
        "name": "eth1/14/1",
        "controllingPort": 9,
        "pins": [
          {
            "a": {
              "chip": "BC7",
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
              "chip": "BC7",
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
              "chip": "BC7",
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
              "chip": "BC7",
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
              "chip": "BC7",
              "lane": 4
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
              "chip": "BC7",
              "lane": 5
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
              "chip": "BC7",
              "lane": 6
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
              "chip": "BC7",
              "lane": 7
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
        "attachedCoreId": 1,
        "attachedCorePortIndex": 9
      },
      "supportedProfiles": {
        "38": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC7",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC7",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC7",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC7",
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
                  "chip": "BC7",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC7",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC7",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC7",
                  "lane": 3
                }
              },
              {
                "id": {
                  "chip": "BC7",
                  "lane": 4
                }
              },
              {
                "id": {
                  "chip": "BC7",
                  "lane": 5
                }
              },
              {
                "id": {
                  "chip": "BC7",
                  "lane": 6
                }
              },
              {
                "id": {
                  "chip": "BC7",
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
        "name": "eth1/12/1",
        "controllingPort": 10,
        "pins": [
          {
            "a": {
              "chip": "BC8",
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
              "chip": "BC8",
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
              "chip": "BC8",
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
              "chip": "BC8",
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
              "chip": "BC8",
              "lane": 4
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
              "chip": "BC8",
              "lane": 5
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
              "chip": "BC8",
              "lane": 6
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
              "chip": "BC8",
              "lane": 7
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
        "attachedCoreId": 1,
        "attachedCorePortIndex": 10
      },
      "supportedProfiles": {
        "38": {
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
            ]
          }
        },
        "39": {
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
              },
              {
                "id": {
                  "chip": "BC8",
                  "lane": 4
                }
              },
              {
                "id": {
                  "chip": "BC8",
                  "lane": 5
                }
              },
              {
                "id": {
                  "chip": "BC8",
                  "lane": 6
                }
              },
              {
                "id": {
                  "chip": "BC8",
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
        "name": "eth1/20/1",
        "controllingPort": 11,
        "pins": [
          {
            "a": {
              "chip": "BC9",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/20",
                "lane": 0
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
                "chip": "eth1/20",
                "lane": 1
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
                "chip": "eth1/20",
                "lane": 2
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
                "chip": "eth1/20",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC9",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "eth1/20",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC9",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "eth1/20",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC9",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "eth1/20",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC9",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "eth1/20",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 2,
        "attachedCorePortIndex": 11
      },
      "supportedProfiles": {
        "38": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC9",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC9",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC9",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC9",
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
                  "chip": "BC9",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC9",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC9",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC9",
                  "lane": 3
                }
              },
              {
                "id": {
                  "chip": "BC9",
                  "lane": 4
                }
              },
              {
                "id": {
                  "chip": "BC9",
                  "lane": 5
                }
              },
              {
                "id": {
                  "chip": "BC9",
                  "lane": 6
                }
              },
              {
                "id": {
                  "chip": "BC9",
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
        "name": "eth1/21/1",
        "controllingPort": 12,
        "pins": [
          {
            "a": {
              "chip": "BC10",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/21",
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
                "chip": "eth1/21",
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
                "chip": "eth1/21",
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
                "chip": "eth1/21",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC10",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "eth1/21",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC10",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "eth1/21",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC10",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "eth1/21",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC10",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "eth1/21",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 2,
        "attachedCorePortIndex": 12
      },
      "supportedProfiles": {
        "38": {
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
            ]
          }
        },
        "39": {
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
              },
              {
                "id": {
                  "chip": "BC10",
                  "lane": 4
                }
              },
              {
                "id": {
                  "chip": "BC10",
                  "lane": 5
                }
              },
              {
                "id": {
                  "chip": "BC10",
                  "lane": 6
                }
              },
              {
                "id": {
                  "chip": "BC10",
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
        "name": "eth1/22/1",
        "controllingPort": 13,
        "pins": [
          {
            "a": {
              "chip": "BC11",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/22",
                "lane": 0
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
                "chip": "eth1/22",
                "lane": 1
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
                "chip": "eth1/22",
                "lane": 2
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
                "chip": "eth1/22",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC11",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "eth1/22",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC11",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "eth1/22",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC11",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "eth1/22",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC11",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "eth1/22",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 2,
        "attachedCorePortIndex": 13
      },
      "supportedProfiles": {
        "38": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC11",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC11",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC11",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC11",
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
                  "chip": "BC11",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC11",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC11",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC11",
                  "lane": 3
                }
              },
              {
                "id": {
                  "chip": "BC11",
                  "lane": 4
                }
              },
              {
                "id": {
                  "chip": "BC11",
                  "lane": 5
                }
              },
              {
                "id": {
                  "chip": "BC11",
                  "lane": 6
                }
              },
              {
                "id": {
                  "chip": "BC11",
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
        "name": "eth1/23/1",
        "controllingPort": 14,
        "pins": [
          {
            "a": {
              "chip": "BC12",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/23",
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
                "chip": "eth1/23",
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
                "chip": "eth1/23",
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
                "chip": "eth1/23",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC12",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "eth1/23",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC12",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "eth1/23",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC12",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "eth1/23",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC12",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "eth1/23",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 2,
        "attachedCorePortIndex": 14
      },
      "supportedProfiles": {
        "38": {
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
            ]
          }
        },
        "39": {
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
              },
              {
                "id": {
                  "chip": "BC12",
                  "lane": 4
                }
              },
              {
                "id": {
                  "chip": "BC12",
                  "lane": 5
                }
              },
              {
                "id": {
                  "chip": "BC12",
                  "lane": 6
                }
              },
              {
                "id": {
                  "chip": "BC12",
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
        "name": "eth1/27/1",
        "controllingPort": 15,
        "pins": [
          {
            "a": {
              "chip": "BC13",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/27",
                "lane": 0
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
                "chip": "eth1/27",
                "lane": 1
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
                "chip": "eth1/27",
                "lane": 2
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
                "chip": "eth1/27",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC13",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "eth1/27",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC13",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "eth1/27",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC13",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "eth1/27",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC13",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "eth1/27",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 3,
        "attachedCorePortIndex": 15
      },
      "supportedProfiles": {
        "38": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC13",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC13",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC13",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC13",
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
                  "chip": "BC13",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC13",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC13",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC13",
                  "lane": 3
                }
              },
              {
                "id": {
                  "chip": "BC13",
                  "lane": 4
                }
              },
              {
                "id": {
                  "chip": "BC13",
                  "lane": 5
                }
              },
              {
                "id": {
                  "chip": "BC13",
                  "lane": 6
                }
              },
              {
                "id": {
                  "chip": "BC13",
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
        "name": "eth1/28/1",
        "controllingPort": 16,
        "pins": [
          {
            "a": {
              "chip": "BC14",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/28",
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
                "chip": "eth1/28",
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
                "chip": "eth1/28",
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
                "chip": "eth1/28",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC14",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "eth1/28",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC14",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "eth1/28",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC14",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "eth1/28",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC14",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "eth1/28",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 3,
        "attachedCorePortIndex": 16
      },
      "supportedProfiles": {
        "38": {
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
            ]
          }
        },
        "39": {
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
              },
              {
                "id": {
                  "chip": "BC14",
                  "lane": 4
                }
              },
              {
                "id": {
                  "chip": "BC14",
                  "lane": 5
                }
              },
              {
                "id": {
                  "chip": "BC14",
                  "lane": 6
                }
              },
              {
                "id": {
                  "chip": "BC14",
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
        "name": "eth1/25/1",
        "controllingPort": 17,
        "pins": [
          {
            "a": {
              "chip": "BC15",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/25",
                "lane": 0
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
                "chip": "eth1/25",
                "lane": 1
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
                "chip": "eth1/25",
                "lane": 2
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
                "chip": "eth1/25",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC15",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "eth1/25",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC15",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "eth1/25",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC15",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "eth1/25",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC15",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "eth1/25",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 3,
        "attachedCorePortIndex": 17
      },
      "supportedProfiles": {
        "38": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC15",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC15",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC15",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC15",
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
                  "chip": "BC15",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC15",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC15",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC15",
                  "lane": 3
                }
              },
              {
                "id": {
                  "chip": "BC15",
                  "lane": 4
                }
              },
              {
                "id": {
                  "chip": "BC15",
                  "lane": 5
                }
              },
              {
                "id": {
                  "chip": "BC15",
                  "lane": 6
                }
              },
              {
                "id": {
                  "chip": "BC15",
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
        "name": "eth1/26/1",
        "controllingPort": 18,
        "pins": [
          {
            "a": {
              "chip": "BC16",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/26",
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
                "chip": "eth1/26",
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
                "chip": "eth1/26",
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
                "chip": "eth1/26",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC16",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "eth1/26",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC16",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "eth1/26",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC16",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "eth1/26",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC16",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "eth1/26",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 3,
        "attachedCorePortIndex": 18
      },
      "supportedProfiles": {
        "38": {
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
            ]
          }
        },
        "39": {
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
              },
              {
                "id": {
                  "chip": "BC16",
                  "lane": 4
                }
              },
              {
                "id": {
                  "chip": "BC16",
                  "lane": 5
                }
              },
              {
                "id": {
                  "chip": "BC16",
                  "lane": 6
                }
              },
              {
                "id": {
                  "chip": "BC16",
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
        "name": "eth1/24/1",
        "controllingPort": 19,
        "pins": [
          {
            "a": {
              "chip": "BC17",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "eth1/24",
                "lane": 0
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
                "chip": "eth1/24",
                "lane": 1
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
                "chip": "eth1/24",
                "lane": 2
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
                "chip": "eth1/24",
                "lane": 3
              }
            }
          },
          {
            "a": {
              "chip": "BC17",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "eth1/24",
                "lane": 4
              }
            }
          },
          {
            "a": {
              "chip": "BC17",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "eth1/24",
                "lane": 5
              }
            }
          },
          {
            "a": {
              "chip": "BC17",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "eth1/24",
                "lane": 6
              }
            }
          },
          {
            "a": {
              "chip": "BC17",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "eth1/24",
                "lane": 7
              }
            }
          }
        ],
        "portType": 0,
        "attachedCoreId": 3,
        "attachedCorePortIndex": 19
      },
      "supportedProfiles": {
        "38": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC17",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC17",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC17",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC17",
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
                  "chip": "BC17",
                  "lane": 0
                }
              },
              {
                "id": {
                  "chip": "BC17",
                  "lane": 1
                }
              },
              {
                "id": {
                  "chip": "BC17",
                  "lane": 2
                }
              },
              {
                "id": {
                  "chip": "BC17",
                  "lane": 3
                }
              },
              {
                "id": {
                  "chip": "BC17",
                  "lane": 4
                }
              },
              {
                "id": {
                  "chip": "BC17",
                  "lane": 5
                }
              },
              {
                "id": {
                  "chip": "BC17",
                  "lane": 6
                }
              },
              {
                "id": {
                  "chip": "BC17",
                  "lane": 7
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
        "name": "fab1/10/1",
        "controllingPort": 256,
        "pins": [
          {
            "a": {
              "chip": "BC0",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/10",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC0",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC0",
                  "lane": 0
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
        "name": "fab1/10/2",
        "controllingPort": 257,
        "pins": [
          {
            "a": {
              "chip": "BC0",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/10",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC0",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC0",
                  "lane": 1
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
        "name": "fab1/10/3",
        "controllingPort": 258,
        "pins": [
          {
            "a": {
              "chip": "BC0",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/10",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC0",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC0",
                  "lane": 2
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
        "name": "fab1/10/4",
        "controllingPort": 259,
        "pins": [
          {
            "a": {
              "chip": "BC0",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/10",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC0",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC0",
                  "lane": 3
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
        "name": "fab1/10/5",
        "controllingPort": 260,
        "pins": [
          {
            "a": {
              "chip": "BC0",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/10",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC0",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC0",
                  "lane": 4
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
        "name": "fab1/10/6",
        "controllingPort": 261,
        "pins": [
          {
            "a": {
              "chip": "BC0",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/10",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC0",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC0",
                  "lane": 5
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
        "name": "fab1/10/7",
        "controllingPort": 262,
        "pins": [
          {
            "a": {
              "chip": "BC0",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/10",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC0",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC0",
                  "lane": 6
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
        "name": "fab1/10/8",
        "controllingPort": 263,
        "pins": [
          {
            "a": {
              "chip": "BC0",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/10",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC0",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC0",
                  "lane": 7
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
        "name": "fab1/9/1",
        "controllingPort": 264,
        "pins": [
          {
            "a": {
              "chip": "BC1",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/9",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC1",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC1",
                  "lane": 0
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
        "name": "fab1/9/2",
        "controllingPort": 265,
        "pins": [
          {
            "a": {
              "chip": "BC1",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/9",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC1",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC1",
                  "lane": 1
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
        "name": "fab1/9/3",
        "controllingPort": 266,
        "pins": [
          {
            "a": {
              "chip": "BC1",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/9",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC1",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC1",
                  "lane": 2
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
        "name": "fab1/9/4",
        "controllingPort": 267,
        "pins": [
          {
            "a": {
              "chip": "BC1",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/9",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC1",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC1",
                  "lane": 3
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
        "name": "fab1/9/5",
        "controllingPort": 268,
        "pins": [
          {
            "a": {
              "chip": "BC1",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/9",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC1",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC1",
                  "lane": 4
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
        "name": "fab1/9/6",
        "controllingPort": 269,
        "pins": [
          {
            "a": {
              "chip": "BC1",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/9",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC1",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC1",
                  "lane": 5
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
        "name": "fab1/9/7",
        "controllingPort": 270,
        "pins": [
          {
            "a": {
              "chip": "BC1",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/9",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC1",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC1",
                  "lane": 6
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
        "name": "fab1/9/8",
        "controllingPort": 271,
        "pins": [
          {
            "a": {
              "chip": "BC1",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/9",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC1",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC1",
                  "lane": 7
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
        "name": "fab1/8/1",
        "controllingPort": 272,
        "pins": [
          {
            "a": {
              "chip": "BC2",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/8",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC2",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC2",
                  "lane": 0
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
        "name": "fab1/8/2",
        "controllingPort": 273,
        "pins": [
          {
            "a": {
              "chip": "BC2",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/8",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC2",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC2",
                  "lane": 1
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
        "name": "fab1/8/3",
        "controllingPort": 274,
        "pins": [
          {
            "a": {
              "chip": "BC2",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/8",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC2",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC2",
                  "lane": 2
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
        "name": "fab1/8/4",
        "controllingPort": 275,
        "pins": [
          {
            "a": {
              "chip": "BC2",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/8",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC2",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC2",
                  "lane": 3
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
        "name": "fab1/8/5",
        "controllingPort": 276,
        "pins": [
          {
            "a": {
              "chip": "BC2",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/8",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC2",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC2",
                  "lane": 4
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
        "name": "fab1/8/6",
        "controllingPort": 277,
        "pins": [
          {
            "a": {
              "chip": "BC2",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/8",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC2",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC2",
                  "lane": 5
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
        "name": "fab1/8/7",
        "controllingPort": 278,
        "pins": [
          {
            "a": {
              "chip": "BC2",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/8",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC2",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC2",
                  "lane": 6
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
        "name": "fab1/8/8",
        "controllingPort": 279,
        "pins": [
          {
            "a": {
              "chip": "BC2",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/8",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC2",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC2",
                  "lane": 7
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
        "name": "fab1/7/1",
        "controllingPort": 280,
        "pins": [
          {
            "a": {
              "chip": "BC3",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/7",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC3",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC3",
                  "lane": 0
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
        "name": "fab1/7/2",
        "controllingPort": 281,
        "pins": [
          {
            "a": {
              "chip": "BC3",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/7",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC3",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC3",
                  "lane": 1
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
        "name": "fab1/7/3",
        "controllingPort": 282,
        "pins": [
          {
            "a": {
              "chip": "BC3",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/7",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC3",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC3",
                  "lane": 2
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
        "name": "fab1/7/4",
        "controllingPort": 283,
        "pins": [
          {
            "a": {
              "chip": "BC3",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/7",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC3",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC3",
                  "lane": 3
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
        "name": "fab1/7/5",
        "controllingPort": 284,
        "pins": [
          {
            "a": {
              "chip": "BC3",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/7",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC3",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC3",
                  "lane": 4
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
        "name": "fab1/7/6",
        "controllingPort": 285,
        "pins": [
          {
            "a": {
              "chip": "BC3",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/7",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC3",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC3",
                  "lane": 5
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
        "name": "fab1/7/7",
        "controllingPort": 286,
        "pins": [
          {
            "a": {
              "chip": "BC3",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/7",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC3",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC3",
                  "lane": 6
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
        "name": "fab1/7/8",
        "controllingPort": 287,
        "pins": [
          {
            "a": {
              "chip": "BC3",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/7",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC3",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC3",
                  "lane": 7
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
        "name": "fab1/6/1",
        "controllingPort": 288,
        "pins": [
          {
            "a": {
              "chip": "BC4",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/6",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC4",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC4",
                  "lane": 0
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
        "name": "fab1/6/2",
        "controllingPort": 289,
        "pins": [
          {
            "a": {
              "chip": "BC4",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/6",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC4",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC4",
                  "lane": 1
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
        "name": "fab1/6/3",
        "controllingPort": 290,
        "pins": [
          {
            "a": {
              "chip": "BC4",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/6",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC4",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC4",
                  "lane": 2
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
        "name": "fab1/6/4",
        "controllingPort": 291,
        "pins": [
          {
            "a": {
              "chip": "BC4",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/6",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC4",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC4",
                  "lane": 3
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
        "name": "fab1/6/5",
        "controllingPort": 292,
        "pins": [
          {
            "a": {
              "chip": "BC4",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/6",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC4",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC4",
                  "lane": 4
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
        "name": "fab1/6/6",
        "controllingPort": 293,
        "pins": [
          {
            "a": {
              "chip": "BC4",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/6",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC4",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC4",
                  "lane": 5
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
        "name": "fab1/6/7",
        "controllingPort": 294,
        "pins": [
          {
            "a": {
              "chip": "BC4",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/6",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC4",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC4",
                  "lane": 6
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
        "name": "fab1/6/8",
        "controllingPort": 295,
        "pins": [
          {
            "a": {
              "chip": "BC4",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/6",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC4",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC4",
                  "lane": 7
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
        "name": "fab1/5/1",
        "controllingPort": 296,
        "pins": [
          {
            "a": {
              "chip": "BC5",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/5",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC5",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC5",
                  "lane": 0
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
        "name": "fab1/5/2",
        "controllingPort": 297,
        "pins": [
          {
            "a": {
              "chip": "BC5",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/5",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC5",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC5",
                  "lane": 1
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
        "name": "fab1/5/3",
        "controllingPort": 298,
        "pins": [
          {
            "a": {
              "chip": "BC5",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/5",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC5",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC5",
                  "lane": 2
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
        "name": "fab1/5/4",
        "controllingPort": 299,
        "pins": [
          {
            "a": {
              "chip": "BC5",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/5",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC5",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC5",
                  "lane": 3
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
        "name": "fab1/5/5",
        "controllingPort": 300,
        "pins": [
          {
            "a": {
              "chip": "BC5",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/5",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC5",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC5",
                  "lane": 4
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
        "name": "fab1/5/6",
        "controllingPort": 301,
        "pins": [
          {
            "a": {
              "chip": "BC5",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/5",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC5",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC5",
                  "lane": 5
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
        "name": "fab1/5/7",
        "controllingPort": 302,
        "pins": [
          {
            "a": {
              "chip": "BC5",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/5",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC5",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC5",
                  "lane": 6
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
        "name": "fab1/5/8",
        "controllingPort": 303,
        "pins": [
          {
            "a": {
              "chip": "BC5",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/5",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC5",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC5",
                  "lane": 7
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
        "name": "fab1/4/1",
        "controllingPort": 304,
        "pins": [
          {
            "a": {
              "chip": "BC6",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/4",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC6",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC6",
                  "lane": 0
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
        "name": "fab1/4/2",
        "controllingPort": 305,
        "pins": [
          {
            "a": {
              "chip": "BC6",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/4",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC6",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC6",
                  "lane": 1
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
        "name": "fab1/4/3",
        "controllingPort": 306,
        "pins": [
          {
            "a": {
              "chip": "BC6",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/4",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC6",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC6",
                  "lane": 2
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
        "name": "fab1/4/4",
        "controllingPort": 307,
        "pins": [
          {
            "a": {
              "chip": "BC6",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/4",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC6",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC6",
                  "lane": 3
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
        "name": "fab1/4/5",
        "controllingPort": 308,
        "pins": [
          {
            "a": {
              "chip": "BC6",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/4",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC6",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC6",
                  "lane": 4
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
        "name": "fab1/4/6",
        "controllingPort": 309,
        "pins": [
          {
            "a": {
              "chip": "BC6",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/4",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC6",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC6",
                  "lane": 5
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
        "name": "fab1/4/7",
        "controllingPort": 310,
        "pins": [
          {
            "a": {
              "chip": "BC6",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/4",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC6",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC6",
                  "lane": 6
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
        "name": "fab1/4/8",
        "controllingPort": 311,
        "pins": [
          {
            "a": {
              "chip": "BC6",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/4",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC6",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC6",
                  "lane": 7
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
        "name": "fab1/3/1",
        "controllingPort": 312,
        "pins": [
          {
            "a": {
              "chip": "BC7",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/3",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC7",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC7",
                  "lane": 0
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
        "name": "fab1/3/2",
        "controllingPort": 313,
        "pins": [
          {
            "a": {
              "chip": "BC7",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/3",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC7",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC7",
                  "lane": 1
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
        "name": "fab1/3/3",
        "controllingPort": 314,
        "pins": [
          {
            "a": {
              "chip": "BC7",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/3",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC7",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC7",
                  "lane": 2
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
        "name": "fab1/3/4",
        "controllingPort": 315,
        "pins": [
          {
            "a": {
              "chip": "BC7",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/3",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC7",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC7",
                  "lane": 3
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
        "name": "fab1/3/5",
        "controllingPort": 316,
        "pins": [
          {
            "a": {
              "chip": "BC7",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/3",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC7",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC7",
                  "lane": 4
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
        "name": "fab1/3/6",
        "controllingPort": 317,
        "pins": [
          {
            "a": {
              "chip": "BC7",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/3",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC7",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC7",
                  "lane": 5
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
        "name": "fab1/3/7",
        "controllingPort": 318,
        "pins": [
          {
            "a": {
              "chip": "BC7",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/3",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC7",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC7",
                  "lane": 6
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
        "name": "fab1/3/8",
        "controllingPort": 319,
        "pins": [
          {
            "a": {
              "chip": "BC7",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/3",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC7",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC7",
                  "lane": 7
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
        "name": "fab1/1/1",
        "controllingPort": 320,
        "pins": [
          {
            "a": {
              "chip": "BC8",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/1",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC8",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC8",
                  "lane": 0
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
        "name": "fab1/1/2",
        "controllingPort": 321,
        "pins": [
          {
            "a": {
              "chip": "BC8",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/1",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC8",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC8",
                  "lane": 1
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
        "name": "fab1/1/3",
        "controllingPort": 322,
        "pins": [
          {
            "a": {
              "chip": "BC8",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/1",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC8",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC8",
                  "lane": 2
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
        "name": "fab1/1/4",
        "controllingPort": 323,
        "pins": [
          {
            "a": {
              "chip": "BC8",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/1",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC8",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC8",
                  "lane": 3
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
        "name": "fab1/1/5",
        "controllingPort": 324,
        "pins": [
          {
            "a": {
              "chip": "BC8",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/1",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC8",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC8",
                  "lane": 4
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
        "name": "fab1/1/6",
        "controllingPort": 325,
        "pins": [
          {
            "a": {
              "chip": "BC8",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/1",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC8",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC8",
                  "lane": 5
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
        "name": "fab1/1/7",
        "controllingPort": 326,
        "pins": [
          {
            "a": {
              "chip": "BC8",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/1",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC8",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC8",
                  "lane": 6
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
        "name": "fab1/1/8",
        "controllingPort": 327,
        "pins": [
          {
            "a": {
              "chip": "BC8",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/1",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC8",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC8",
                  "lane": 7
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
        "name": "fab1/2/1",
        "controllingPort": 328,
        "pins": [
          {
            "a": {
              "chip": "BC9",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/2",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC9",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC9",
                  "lane": 0
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
        "name": "fab1/2/2",
        "controllingPort": 329,
        "pins": [
          {
            "a": {
              "chip": "BC9",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/2",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC9",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC9",
                  "lane": 1
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
        "name": "fab1/2/3",
        "controllingPort": 330,
        "pins": [
          {
            "a": {
              "chip": "BC9",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/2",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC9",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC9",
                  "lane": 2
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
        "name": "fab1/2/4",
        "controllingPort": 331,
        "pins": [
          {
            "a": {
              "chip": "BC9",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/2",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC9",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC9",
                  "lane": 3
                }
              }
            ]
          }
        }
      }
    },
    "332": {
      "mapping": {
        "id": 332,
        "name": "fab1/2/5",
        "controllingPort": 332,
        "pins": [
          {
            "a": {
              "chip": "BC9",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/2",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC9",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC9",
                  "lane": 4
                }
              }
            ]
          }
        }
      }
    },
    "333": {
      "mapping": {
        "id": 333,
        "name": "fab1/2/6",
        "controllingPort": 333,
        "pins": [
          {
            "a": {
              "chip": "BC9",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/2",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC9",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC9",
                  "lane": 5
                }
              }
            ]
          }
        }
      }
    },
    "334": {
      "mapping": {
        "id": 334,
        "name": "fab1/2/7",
        "controllingPort": 334,
        "pins": [
          {
            "a": {
              "chip": "BC9",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/2",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC9",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC9",
                  "lane": 6
                }
              }
            ]
          }
        }
      }
    },
    "335": {
      "mapping": {
        "id": 335,
        "name": "fab1/2/8",
        "controllingPort": 335,
        "pins": [
          {
            "a": {
              "chip": "BC9",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/2",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC9",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC9",
                  "lane": 7
                }
              }
            ]
          }
        }
      }
    },
    "336": {
      "mapping": {
        "id": 336,
        "name": "fab1/30/1",
        "controllingPort": 336,
        "pins": [
          {
            "a": {
              "chip": "BC10",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/30",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC10",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC10",
                  "lane": 0
                }
              }
            ]
          }
        }
      }
    },
    "337": {
      "mapping": {
        "id": 337,
        "name": "fab1/30/2",
        "controllingPort": 337,
        "pins": [
          {
            "a": {
              "chip": "BC10",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/30",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC10",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC10",
                  "lane": 1
                }
              }
            ]
          }
        }
      }
    },
    "338": {
      "mapping": {
        "id": 338,
        "name": "fab1/30/3",
        "controllingPort": 338,
        "pins": [
          {
            "a": {
              "chip": "BC10",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/30",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC10",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC10",
                  "lane": 2
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
        "name": "fab1/30/4",
        "controllingPort": 339,
        "pins": [
          {
            "a": {
              "chip": "BC10",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/30",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC10",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC10",
                  "lane": 3
                }
              }
            ]
          }
        }
      }
    },
    "340": {
      "mapping": {
        "id": 340,
        "name": "fab1/30/5",
        "controllingPort": 340,
        "pins": [
          {
            "a": {
              "chip": "BC10",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/30",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC10",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC10",
                  "lane": 4
                }
              }
            ]
          }
        }
      }
    },
    "341": {
      "mapping": {
        "id": 341,
        "name": "fab1/30/6",
        "controllingPort": 341,
        "pins": [
          {
            "a": {
              "chip": "BC10",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/30",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC10",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC10",
                  "lane": 5
                }
              }
            ]
          }
        }
      }
    },
    "342": {
      "mapping": {
        "id": 342,
        "name": "fab1/30/7",
        "controllingPort": 342,
        "pins": [
          {
            "a": {
              "chip": "BC10",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/30",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC10",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC10",
                  "lane": 6
                }
              }
            ]
          }
        }
      }
    },
    "343": {
      "mapping": {
        "id": 343,
        "name": "fab1/30/8",
        "controllingPort": 343,
        "pins": [
          {
            "a": {
              "chip": "BC10",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/30",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC10",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC10",
                  "lane": 7
                }
              }
            ]
          }
        }
      }
    },
    "344": {
      "mapping": {
        "id": 344,
        "name": "fab1/29/1",
        "controllingPort": 344,
        "pins": [
          {
            "a": {
              "chip": "BC11",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/29",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC11",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC11",
                  "lane": 0
                }
              }
            ]
          }
        }
      }
    },
    "345": {
      "mapping": {
        "id": 345,
        "name": "fab1/29/2",
        "controllingPort": 345,
        "pins": [
          {
            "a": {
              "chip": "BC11",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/29",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC11",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC11",
                  "lane": 1
                }
              }
            ]
          }
        }
      }
    },
    "346": {
      "mapping": {
        "id": 346,
        "name": "fab1/29/3",
        "controllingPort": 346,
        "pins": [
          {
            "a": {
              "chip": "BC11",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/29",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC11",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC11",
                  "lane": 2
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
        "name": "fab1/29/4",
        "controllingPort": 347,
        "pins": [
          {
            "a": {
              "chip": "BC11",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/29",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC11",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC11",
                  "lane": 3
                }
              }
            ]
          }
        }
      }
    },
    "348": {
      "mapping": {
        "id": 348,
        "name": "fab1/29/5",
        "controllingPort": 348,
        "pins": [
          {
            "a": {
              "chip": "BC11",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/29",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC11",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC11",
                  "lane": 4
                }
              }
            ]
          }
        }
      }
    },
    "349": {
      "mapping": {
        "id": 349,
        "name": "fab1/29/6",
        "controllingPort": 349,
        "pins": [
          {
            "a": {
              "chip": "BC11",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/29",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC11",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC11",
                  "lane": 5
                }
              }
            ]
          }
        }
      }
    },
    "350": {
      "mapping": {
        "id": 350,
        "name": "fab1/29/7",
        "controllingPort": 350,
        "pins": [
          {
            "a": {
              "chip": "BC11",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/29",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC11",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC11",
                  "lane": 6
                }
              }
            ]
          }
        }
      }
    },
    "351": {
      "mapping": {
        "id": 351,
        "name": "fab1/29/8",
        "controllingPort": 351,
        "pins": [
          {
            "a": {
              "chip": "BC11",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/29",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC11",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC11",
                  "lane": 7
                }
              }
            ]
          }
        }
      }
    },
    "352": {
      "mapping": {
        "id": 352,
        "name": "fab1/32/1",
        "controllingPort": 352,
        "pins": [
          {
            "a": {
              "chip": "BC12",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/32",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC12",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC12",
                  "lane": 0
                }
              }
            ]
          }
        }
      }
    },
    "353": {
      "mapping": {
        "id": 353,
        "name": "fab1/32/2",
        "controllingPort": 353,
        "pins": [
          {
            "a": {
              "chip": "BC12",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/32",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC12",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC12",
                  "lane": 1
                }
              }
            ]
          }
        }
      }
    },
    "354": {
      "mapping": {
        "id": 354,
        "name": "fab1/32/3",
        "controllingPort": 354,
        "pins": [
          {
            "a": {
              "chip": "BC12",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/32",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC12",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC12",
                  "lane": 2
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
        "name": "fab1/32/4",
        "controllingPort": 355,
        "pins": [
          {
            "a": {
              "chip": "BC12",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/32",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC12",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC12",
                  "lane": 3
                }
              }
            ]
          }
        }
      }
    },
    "356": {
      "mapping": {
        "id": 356,
        "name": "fab1/32/5",
        "controllingPort": 356,
        "pins": [
          {
            "a": {
              "chip": "BC12",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/32",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC12",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC12",
                  "lane": 4
                }
              }
            ]
          }
        }
      }
    },
    "357": {
      "mapping": {
        "id": 357,
        "name": "fab1/32/6",
        "controllingPort": 357,
        "pins": [
          {
            "a": {
              "chip": "BC12",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/32",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC12",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC12",
                  "lane": 5
                }
              }
            ]
          }
        }
      }
    },
    "358": {
      "mapping": {
        "id": 358,
        "name": "fab1/32/7",
        "controllingPort": 358,
        "pins": [
          {
            "a": {
              "chip": "BC12",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/32",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC12",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC12",
                  "lane": 6
                }
              }
            ]
          }
        }
      }
    },
    "359": {
      "mapping": {
        "id": 359,
        "name": "fab1/32/8",
        "controllingPort": 359,
        "pins": [
          {
            "a": {
              "chip": "BC12",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/32",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC12",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC12",
                  "lane": 7
                }
              }
            ]
          }
        }
      }
    },
    "360": {
      "mapping": {
        "id": 360,
        "name": "fab1/33/1",
        "controllingPort": 360,
        "pins": [
          {
            "a": {
              "chip": "BC13",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/33",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC13",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC13",
                  "lane": 0
                }
              }
            ]
          }
        }
      }
    },
    "361": {
      "mapping": {
        "id": 361,
        "name": "fab1/33/2",
        "controllingPort": 361,
        "pins": [
          {
            "a": {
              "chip": "BC13",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/33",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC13",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC13",
                  "lane": 1
                }
              }
            ]
          }
        }
      }
    },
    "362": {
      "mapping": {
        "id": 362,
        "name": "fab1/33/3",
        "controllingPort": 362,
        "pins": [
          {
            "a": {
              "chip": "BC13",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/33",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC13",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC13",
                  "lane": 2
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
        "name": "fab1/33/4",
        "controllingPort": 363,
        "pins": [
          {
            "a": {
              "chip": "BC13",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/33",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC13",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC13",
                  "lane": 3
                }
              }
            ]
          }
        }
      }
    },
    "364": {
      "mapping": {
        "id": 364,
        "name": "fab1/33/5",
        "controllingPort": 364,
        "pins": [
          {
            "a": {
              "chip": "BC13",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/33",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC13",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC13",
                  "lane": 4
                }
              }
            ]
          }
        }
      }
    },
    "365": {
      "mapping": {
        "id": 365,
        "name": "fab1/33/6",
        "controllingPort": 365,
        "pins": [
          {
            "a": {
              "chip": "BC13",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/33",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC13",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC13",
                  "lane": 5
                }
              }
            ]
          }
        }
      }
    },
    "366": {
      "mapping": {
        "id": 366,
        "name": "fab1/33/7",
        "controllingPort": 366,
        "pins": [
          {
            "a": {
              "chip": "BC13",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/33",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC13",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC13",
                  "lane": 6
                }
              }
            ]
          }
        }
      }
    },
    "367": {
      "mapping": {
        "id": 367,
        "name": "fab1/33/8",
        "controllingPort": 367,
        "pins": [
          {
            "a": {
              "chip": "BC13",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/33",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC13",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC13",
                  "lane": 7
                }
              }
            ]
          }
        }
      }
    },
    "368": {
      "mapping": {
        "id": 368,
        "name": "fab1/31/1",
        "controllingPort": 368,
        "pins": [
          {
            "a": {
              "chip": "BC14",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/31",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC14",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC14",
                  "lane": 0
                }
              }
            ]
          }
        }
      }
    },
    "369": {
      "mapping": {
        "id": 369,
        "name": "fab1/31/2",
        "controllingPort": 369,
        "pins": [
          {
            "a": {
              "chip": "BC14",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/31",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC14",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC14",
                  "lane": 1
                }
              }
            ]
          }
        }
      }
    },
    "370": {
      "mapping": {
        "id": 370,
        "name": "fab1/31/3",
        "controllingPort": 370,
        "pins": [
          {
            "a": {
              "chip": "BC14",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/31",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC14",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC14",
                  "lane": 2
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
        "name": "fab1/31/4",
        "controllingPort": 371,
        "pins": [
          {
            "a": {
              "chip": "BC14",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/31",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC14",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC14",
                  "lane": 3
                }
              }
            ]
          }
        }
      }
    },
    "372": {
      "mapping": {
        "id": 372,
        "name": "fab1/31/5",
        "controllingPort": 372,
        "pins": [
          {
            "a": {
              "chip": "BC14",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/31",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC14",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC14",
                  "lane": 4
                }
              }
            ]
          }
        }
      }
    },
    "373": {
      "mapping": {
        "id": 373,
        "name": "fab1/31/6",
        "controllingPort": 373,
        "pins": [
          {
            "a": {
              "chip": "BC14",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/31",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC14",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC14",
                  "lane": 5
                }
              }
            ]
          }
        }
      }
    },
    "374": {
      "mapping": {
        "id": 374,
        "name": "fab1/31/7",
        "controllingPort": 374,
        "pins": [
          {
            "a": {
              "chip": "BC14",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/31",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC14",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC14",
                  "lane": 6
                }
              }
            ]
          }
        }
      }
    },
    "375": {
      "mapping": {
        "id": 375,
        "name": "fab1/31/8",
        "controllingPort": 375,
        "pins": [
          {
            "a": {
              "chip": "BC14",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/31",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC14",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC14",
                  "lane": 7
                }
              }
            ]
          }
        }
      }
    },
    "376": {
      "mapping": {
        "id": 376,
        "name": "fab1/34/1",
        "controllingPort": 376,
        "pins": [
          {
            "a": {
              "chip": "BC15",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/34",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC15",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC15",
                  "lane": 0
                }
              }
            ]
          }
        }
      }
    },
    "377": {
      "mapping": {
        "id": 377,
        "name": "fab1/34/2",
        "controllingPort": 377,
        "pins": [
          {
            "a": {
              "chip": "BC15",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/34",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC15",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC15",
                  "lane": 1
                }
              }
            ]
          }
        }
      }
    },
    "378": {
      "mapping": {
        "id": 378,
        "name": "fab1/34/3",
        "controllingPort": 378,
        "pins": [
          {
            "a": {
              "chip": "BC15",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/34",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC15",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC15",
                  "lane": 2
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
        "name": "fab1/34/4",
        "controllingPort": 379,
        "pins": [
          {
            "a": {
              "chip": "BC15",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/34",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC15",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC15",
                  "lane": 3
                }
              }
            ]
          }
        }
      }
    },
    "380": {
      "mapping": {
        "id": 380,
        "name": "fab1/34/5",
        "controllingPort": 380,
        "pins": [
          {
            "a": {
              "chip": "BC15",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/34",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC15",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC15",
                  "lane": 4
                }
              }
            ]
          }
        }
      }
    },
    "381": {
      "mapping": {
        "id": 381,
        "name": "fab1/34/6",
        "controllingPort": 381,
        "pins": [
          {
            "a": {
              "chip": "BC15",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/34",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC15",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC15",
                  "lane": 5
                }
              }
            ]
          }
        }
      }
    },
    "382": {
      "mapping": {
        "id": 382,
        "name": "fab1/34/7",
        "controllingPort": 382,
        "pins": [
          {
            "a": {
              "chip": "BC15",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/34",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC15",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC15",
                  "lane": 6
                }
              }
            ]
          }
        }
      }
    },
    "383": {
      "mapping": {
        "id": 383,
        "name": "fab1/34/8",
        "controllingPort": 383,
        "pins": [
          {
            "a": {
              "chip": "BC15",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/34",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC15",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC15",
                  "lane": 7
                }
              }
            ]
          }
        }
      }
    },
    "384": {
      "mapping": {
        "id": 384,
        "name": "fab1/38/1",
        "controllingPort": 384,
        "pins": [
          {
            "a": {
              "chip": "BC16",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/38",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC16",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC16",
                  "lane": 0
                }
              }
            ]
          }
        }
      }
    },
    "385": {
      "mapping": {
        "id": 385,
        "name": "fab1/38/2",
        "controllingPort": 385,
        "pins": [
          {
            "a": {
              "chip": "BC16",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/38",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC16",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC16",
                  "lane": 1
                }
              }
            ]
          }
        }
      }
    },
    "386": {
      "mapping": {
        "id": 386,
        "name": "fab1/38/3",
        "controllingPort": 386,
        "pins": [
          {
            "a": {
              "chip": "BC16",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/38",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC16",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC16",
                  "lane": 2
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
        "name": "fab1/38/4",
        "controllingPort": 387,
        "pins": [
          {
            "a": {
              "chip": "BC16",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/38",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC16",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC16",
                  "lane": 3
                }
              }
            ]
          }
        }
      }
    },
    "388": {
      "mapping": {
        "id": 388,
        "name": "fab1/38/5",
        "controllingPort": 388,
        "pins": [
          {
            "a": {
              "chip": "BC16",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/38",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC16",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC16",
                  "lane": 4
                }
              }
            ]
          }
        }
      }
    },
    "389": {
      "mapping": {
        "id": 389,
        "name": "fab1/38/6",
        "controllingPort": 389,
        "pins": [
          {
            "a": {
              "chip": "BC16",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/38",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC16",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC16",
                  "lane": 5
                }
              }
            ]
          }
        }
      }
    },
    "390": {
      "mapping": {
        "id": 390,
        "name": "fab1/38/7",
        "controllingPort": 390,
        "pins": [
          {
            "a": {
              "chip": "BC16",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/38",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC16",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC16",
                  "lane": 6
                }
              }
            ]
          }
        }
      }
    },
    "391": {
      "mapping": {
        "id": 391,
        "name": "fab1/38/8",
        "controllingPort": 391,
        "pins": [
          {
            "a": {
              "chip": "BC16",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/38",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC16",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC16",
                  "lane": 7
                }
              }
            ]
          }
        }
      }
    },
    "392": {
      "mapping": {
        "id": 392,
        "name": "fab1/37/1",
        "controllingPort": 392,
        "pins": [
          {
            "a": {
              "chip": "BC17",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/37",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC17",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC17",
                  "lane": 0
                }
              }
            ]
          }
        }
      }
    },
    "393": {
      "mapping": {
        "id": 393,
        "name": "fab1/37/2",
        "controllingPort": 393,
        "pins": [
          {
            "a": {
              "chip": "BC17",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/37",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC17",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC17",
                  "lane": 1
                }
              }
            ]
          }
        }
      }
    },
    "394": {
      "mapping": {
        "id": 394,
        "name": "fab1/37/3",
        "controllingPort": 394,
        "pins": [
          {
            "a": {
              "chip": "BC17",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/37",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC17",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC17",
                  "lane": 2
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
        "name": "fab1/37/4",
        "controllingPort": 395,
        "pins": [
          {
            "a": {
              "chip": "BC17",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/37",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC17",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC17",
                  "lane": 3
                }
              }
            ]
          }
        }
      }
    },
    "396": {
      "mapping": {
        "id": 396,
        "name": "fab1/37/5",
        "controllingPort": 396,
        "pins": [
          {
            "a": {
              "chip": "BC17",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/37",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC17",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC17",
                  "lane": 4
                }
              }
            ]
          }
        }
      }
    },
    "397": {
      "mapping": {
        "id": 397,
        "name": "fab1/37/6",
        "controllingPort": 397,
        "pins": [
          {
            "a": {
              "chip": "BC17",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/37",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC17",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC17",
                  "lane": 5
                }
              }
            ]
          }
        }
      }
    },
    "398": {
      "mapping": {
        "id": 398,
        "name": "fab1/37/7",
        "controllingPort": 398,
        "pins": [
          {
            "a": {
              "chip": "BC17",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/37",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC17",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC17",
                  "lane": 6
                }
              }
            ]
          }
        }
      }
    },
    "399": {
      "mapping": {
        "id": 399,
        "name": "fab1/37/8",
        "controllingPort": 399,
        "pins": [
          {
            "a": {
              "chip": "BC17",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/37",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC17",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC17",
                  "lane": 7
                }
              }
            ]
          }
        }
      }
    },
    "400": {
      "mapping": {
        "id": 400,
        "name": "fab1/35/1",
        "controllingPort": 400,
        "pins": [
          {
            "a": {
              "chip": "BC18",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/35",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC18",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC18",
                  "lane": 0
                }
              }
            ]
          }
        }
      }
    },
    "401": {
      "mapping": {
        "id": 401,
        "name": "fab1/35/2",
        "controllingPort": 401,
        "pins": [
          {
            "a": {
              "chip": "BC18",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/35",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC18",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC18",
                  "lane": 1
                }
              }
            ]
          }
        }
      }
    },
    "402": {
      "mapping": {
        "id": 402,
        "name": "fab1/35/3",
        "controllingPort": 402,
        "pins": [
          {
            "a": {
              "chip": "BC18",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/35",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC18",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC18",
                  "lane": 2
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
        "name": "fab1/35/4",
        "controllingPort": 403,
        "pins": [
          {
            "a": {
              "chip": "BC18",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/35",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC18",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC18",
                  "lane": 3
                }
              }
            ]
          }
        }
      }
    },
    "404": {
      "mapping": {
        "id": 404,
        "name": "fab1/35/5",
        "controllingPort": 404,
        "pins": [
          {
            "a": {
              "chip": "BC18",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/35",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC18",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC18",
                  "lane": 4
                }
              }
            ]
          }
        }
      }
    },
    "405": {
      "mapping": {
        "id": 405,
        "name": "fab1/35/6",
        "controllingPort": 405,
        "pins": [
          {
            "a": {
              "chip": "BC18",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/35",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC18",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC18",
                  "lane": 5
                }
              }
            ]
          }
        }
      }
    },
    "406": {
      "mapping": {
        "id": 406,
        "name": "fab1/35/7",
        "controllingPort": 406,
        "pins": [
          {
            "a": {
              "chip": "BC18",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/35",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC18",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC18",
                  "lane": 6
                }
              }
            ]
          }
        }
      }
    },
    "407": {
      "mapping": {
        "id": 407,
        "name": "fab1/35/8",
        "controllingPort": 407,
        "pins": [
          {
            "a": {
              "chip": "BC18",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/35",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC18",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC18",
                  "lane": 7
                }
              }
            ]
          }
        }
      }
    },
    "408": {
      "mapping": {
        "id": 408,
        "name": "fab1/36/1",
        "controllingPort": 408,
        "pins": [
          {
            "a": {
              "chip": "BC19",
              "lane": 0
            },
            "z": {
              "end": {
                "chip": "fab1/36",
                "lane": 0
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC19",
                  "lane": 0
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC19",
                  "lane": 0
                }
              }
            ]
          }
        }
      }
    },
    "409": {
      "mapping": {
        "id": 409,
        "name": "fab1/36/2",
        "controllingPort": 409,
        "pins": [
          {
            "a": {
              "chip": "BC19",
              "lane": 1
            },
            "z": {
              "end": {
                "chip": "fab1/36",
                "lane": 1
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC19",
                  "lane": 1
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC19",
                  "lane": 1
                }
              }
            ]
          }
        }
      }
    },
    "410": {
      "mapping": {
        "id": 410,
        "name": "fab1/36/3",
        "controllingPort": 410,
        "pins": [
          {
            "a": {
              "chip": "BC19",
              "lane": 2
            },
            "z": {
              "end": {
                "chip": "fab1/36",
                "lane": 2
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC19",
                  "lane": 2
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC19",
                  "lane": 2
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
        "name": "fab1/36/4",
        "controllingPort": 411,
        "pins": [
          {
            "a": {
              "chip": "BC19",
              "lane": 3
            },
            "z": {
              "end": {
                "chip": "fab1/36",
                "lane": 3
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC19",
                  "lane": 3
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC19",
                  "lane": 3
                }
              }
            ]
          }
        }
      }
    },
    "412": {
      "mapping": {
        "id": 412,
        "name": "fab1/36/5",
        "controllingPort": 412,
        "pins": [
          {
            "a": {
              "chip": "BC19",
              "lane": 4
            },
            "z": {
              "end": {
                "chip": "fab1/36",
                "lane": 4
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC19",
                  "lane": 4
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC19",
                  "lane": 4
                }
              }
            ]
          }
        }
      }
    },
    "413": {
      "mapping": {
        "id": 413,
        "name": "fab1/36/6",
        "controllingPort": 413,
        "pins": [
          {
            "a": {
              "chip": "BC19",
              "lane": 5
            },
            "z": {
              "end": {
                "chip": "fab1/36",
                "lane": 5
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC19",
                  "lane": 5
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC19",
                  "lane": 5
                }
              }
            ]
          }
        }
      }
    },
    "414": {
      "mapping": {
        "id": 414,
        "name": "fab1/36/7",
        "controllingPort": 414,
        "pins": [
          {
            "a": {
              "chip": "BC19",
              "lane": 6
            },
            "z": {
              "end": {
                "chip": "fab1/36",
                "lane": 6
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC19",
                  "lane": 6
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC19",
                  "lane": 6
                }
              }
            ]
          }
        }
      }
    },
    "415": {
      "mapping": {
        "id": 415,
        "name": "fab1/36/8",
        "controllingPort": 415,
        "pins": [
          {
            "a": {
              "chip": "BC19",
              "lane": 7
            },
            "z": {
              "end": {
                "chip": "fab1/36",
                "lane": 7
              }
            }
          }
        ],
        "portType": 1
      },
      "supportedProfiles": {
        "36": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC19",
                  "lane": 7
                }
              }
            ]
          }
        },
        "37": {
          "pins": {
            "iphy": [
              {
                "id": {
                  "chip": "BC19",
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
      "name": "fab1/1",
      "type": 3,
      "physicalID": 1
    },
    {
      "name": "fab1/2",
      "type": 3,
      "physicalID": 2
    },
    {
      "name": "fab1/3",
      "type": 3,
      "physicalID": 3
    },
    {
      "name": "fab1/4",
      "type": 3,
      "physicalID": 4
    },
    {
      "name": "fab1/5",
      "type": 3,
      "physicalID": 5
    },
    {
      "name": "fab1/6",
      "type": 3,
      "physicalID": 6
    },
    {
      "name": "fab1/7",
      "type": 3,
      "physicalID": 7
    },
    {
      "name": "fab1/8",
      "type": 3,
      "physicalID": 8
    },
    {
      "name": "fab1/9",
      "type": 3,
      "physicalID": 9
    },
    {
      "name": "fab1/10",
      "type": 3,
      "physicalID": 10
    },
    {
      "name": "eth1/11",
      "type": 3,
      "physicalID": 11
    },
    {
      "name": "eth1/12",
      "type": 3,
      "physicalID": 12
    },
    {
      "name": "eth1/13",
      "type": 3,
      "physicalID": 13
    },
    {
      "name": "eth1/14",
      "type": 3,
      "physicalID": 14
    },
    {
      "name": "eth1/15",
      "type": 3,
      "physicalID": 15
    },
    {
      "name": "eth1/16",
      "type": 3,
      "physicalID": 16
    },
    {
      "name": "eth1/17",
      "type": 3,
      "physicalID": 17
    },
    {
      "name": "eth1/18",
      "type": 3,
      "physicalID": 18
    },
    {
      "name": "eth1/19",
      "type": 3,
      "physicalID": 19
    },
    {
      "name": "eth1/20",
      "type": 3,
      "physicalID": 20
    },
    {
      "name": "eth1/21",
      "type": 3,
      "physicalID": 21
    },
    {
      "name": "eth1/22",
      "type": 3,
      "physicalID": 22
    },
    {
      "name": "eth1/23",
      "type": 3,
      "physicalID": 23
    },
    {
      "name": "eth1/24",
      "type": 3,
      "physicalID": 24
    },
    {
      "name": "eth1/25",
      "type": 3,
      "physicalID": 25
    },
    {
      "name": "eth1/26",
      "type": 3,
      "physicalID": 26
    },
    {
      "name": "eth1/27",
      "type": 3,
      "physicalID": 27
    },
    {
      "name": "eth1/28",
      "type": 3,
      "physicalID": 28
    },
    {
      "name": "fab1/29",
      "type": 3,
      "physicalID": 29
    },
    {
      "name": "fab1/30",
      "type": 3,
      "physicalID": 30
    },
    {
      "name": "fab1/31",
      "type": 3,
      "physicalID": 31
    },
    {
      "name": "fab1/32",
      "type": 3,
      "physicalID": 32
    },
    {
      "name": "fab1/33",
      "type": 3,
      "physicalID": 33
    },
    {
      "name": "fab1/34",
      "type": 3,
      "physicalID": 34
    },
    {
      "name": "fab1/35",
      "type": 3,
      "physicalID": 35
    },
    {
      "name": "fab1/36",
      "type": 3,
      "physicalID": 36
    },
    {
      "name": "fab1/37",
      "type": 3,
      "physicalID": 37
    },
    {
      "name": "fab1/38",
      "type": 3,
      "physicalID": 38
    },
    {
      "name": "rcy1",
      "type": 1,
      "physicalID": 55
    }
  ],
  "platformSettings": {
    "1": "68:00"
  },
  "portConfigOverrides": [],
  "platformSupportedProfiles": [
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
          "interfaceMode": 21,
          "interfaceType": 21
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
          "interfaceMode": 23,
          "interfaceType": 23
        }
      }
    },
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
    }
  ]
}
)";
} // namespace

Meru800biaPlatformMapping::Meru800biaPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

Meru800biaPlatformMapping::Meru800biaPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace facebook::fboss
