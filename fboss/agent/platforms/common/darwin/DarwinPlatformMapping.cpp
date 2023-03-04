/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/darwin/DarwinPlatformMapping.h"

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
                "chip": "BC0",
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
                "chip": "BC0",
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
                "chip": "BC0",
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
                "chip": "BC0",
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
                "chip": "BC0",
                "lane": 4
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
                "chip": "BC0",
                "lane": 5
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
                "chip": "BC0",
                "lane": 6
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
                "chip": "BC0",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 7
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
                      "chip": "BC0",
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
                      "chip": "BC0",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                      "chip": "BC0",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
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
          "name": "eth1/2/1",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "BC1",
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
                "chip": "BC1",
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
                "chip": "BC1",
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
                "chip": "BC1",
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
                "chip": "BC1",
                "lane": 4
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
                "chip": "BC1",
                "lane": 5
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
                "chip": "BC1",
                "lane": 6
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
                "chip": "BC1",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 7
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
                      "chip": "BC1",
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
                      "chip": "BC1",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                      "chip": "BC1",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
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
          "name": "eth1/3/1",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "BC2",
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
                "chip": "BC2",
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
                "chip": "BC2",
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
                "chip": "BC2",
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
                "chip": "BC2",
                "lane": 4
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
                "chip": "BC2",
                "lane": 5
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
                "chip": "BC2",
                "lane": 6
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
                "chip": "BC2",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 7
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
                      "chip": "BC2",
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
                      "chip": "BC2",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                      "chip": "BC2",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
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
          "name": "eth1/4/1",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "BC3",
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
                "chip": "BC3",
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
                "chip": "BC3",
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
                "chip": "BC3",
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
                "chip": "BC3",
                "lane": 4
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
                "chip": "BC3",
                "lane": 5
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
                "chip": "BC3",
                "lane": 6
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
                "chip": "BC3",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 7
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
                      "chip": "BC3",
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
                      "chip": "BC3",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                      "chip": "BC3",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
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
          "name": "eth1/5/1",
          "controllingPort": 20,
          "pins": [
            {
              "a": {
                "chip": "BC4",
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
                "chip": "BC4",
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
                "chip": "BC4",
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
                "chip": "BC4",
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
                "chip": "BC4",
                "lane": 4
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
                "chip": "BC4",
                "lane": 5
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
                "chip": "BC4",
                "lane": 6
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
                "chip": "BC4",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 7
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
                      "chip": "BC4",
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
                      "chip": "BC4",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                      "chip": "BC4",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
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
          "name": "eth1/6/1",
          "controllingPort": 24,
          "pins": [
            {
              "a": {
                "chip": "BC5",
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
                "chip": "BC5",
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
                "chip": "BC5",
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
                "chip": "BC5",
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
                "chip": "BC5",
                "lane": 4
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
                "chip": "BC5",
                "lane": 5
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
                "chip": "BC5",
                "lane": 6
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
                "chip": "BC5",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 7
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
                      "chip": "BC5",
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
                      "chip": "BC5",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                      "chip": "BC5",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
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
          "name": "eth1/7/1",
          "controllingPort": 28,
          "pins": [
            {
              "a": {
                "chip": "BC6",
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
                "chip": "BC6",
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
                "chip": "BC6",
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
                "chip": "BC6",
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
                "chip": "BC6",
                "lane": 4
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
                "chip": "BC6",
                "lane": 5
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
                "chip": "BC6",
                "lane": 6
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
                "chip": "BC6",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 7
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
                      "chip": "BC6",
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
                      "chip": "BC6",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                      "chip": "BC6",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
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
          "name": "eth1/8/1",
          "controllingPort": 32,
          "pins": [
            {
              "a": {
                "chip": "BC7",
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
                "chip": "BC7",
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
                "chip": "BC7",
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
                "chip": "BC7",
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
                "chip": "BC7",
                "lane": 4
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
                "chip": "BC7",
                "lane": 5
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
                "chip": "BC7",
                "lane": 6
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
                "chip": "BC7",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 7
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
                      "chip": "BC7",
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
                      "chip": "BC7",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                      "chip": "BC7",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
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
          "name": "eth1/9/1",
          "controllingPort": 40,
          "pins": [
            {
              "a": {
                "chip": "BC8",
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
                "chip": "BC8",
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
                "chip": "BC8",
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
                "chip": "BC8",
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
                "chip": "BC8",
                "lane": 4
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
                "chip": "BC8",
                "lane": 5
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
                "chip": "BC8",
                "lane": 6
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
                "chip": "BC8",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 7
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
                      "chip": "BC8",
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
                      "chip": "BC8",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                      "chip": "BC8",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
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
          "name": "eth1/10/1",
          "controllingPort": 44,
          "pins": [
            {
              "a": {
                "chip": "BC9",
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
                "chip": "BC9",
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
                "chip": "BC9",
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
                "chip": "BC9",
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
                "chip": "BC9",
                "lane": 4
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
                "chip": "BC9",
                "lane": 5
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
                "chip": "BC9",
                "lane": 6
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
                "chip": "BC9",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 7
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
                      "chip": "BC9",
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
                      "chip": "BC9",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                      "chip": "BC9",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
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
          "name": "eth1/11/1",
          "controllingPort": 48,
          "pins": [
            {
              "a": {
                "chip": "BC10",
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
                "chip": "BC10",
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
                "chip": "BC10",
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
                "chip": "BC10",
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
                "chip": "BC10",
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
                "chip": "BC10",
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
                "chip": "BC10",
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
                "chip": "BC10",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 7
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
                      "chip": "BC10",
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
                      "chip": "BC10",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                      "chip": "BC10",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
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
          "name": "eth1/12/1",
          "controllingPort": 52,
          "pins": [
            {
              "a": {
                "chip": "BC11",
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
                "chip": "BC11",
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
                "chip": "BC11",
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
                "chip": "BC11",
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
                "chip": "BC11",
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
                "chip": "BC11",
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
                "chip": "BC11",
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
                "chip": "BC11",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 7
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
                      "chip": "BC11",
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
                      "chip": "BC11",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                      "chip": "BC11",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
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
          "name": "eth1/13/1",
          "controllingPort": 60,
          "pins": [
            {
              "a": {
                "chip": "BC12",
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
                "chip": "BC12",
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
                "chip": "BC12",
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
                "chip": "BC12",
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
                "chip": "BC12",
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
                "chip": "BC12",
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
                "chip": "BC12",
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
                "chip": "BC12",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 7
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
                      "chip": "BC12",
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
                      "chip": "BC12",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                      "chip": "BC12",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
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
          "name": "eth1/14/1",
          "controllingPort": 64,
          "pins": [
            {
              "a": {
                "chip": "BC13",
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
                "chip": "BC13",
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
                "chip": "BC13",
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
                "chip": "BC13",
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
                "chip": "BC13",
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
                "chip": "BC13",
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
                "chip": "BC13",
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
                "chip": "BC13",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 7
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
                      "chip": "BC13",
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
                      "chip": "BC13",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                      "chip": "BC13",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
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
          "name": "eth1/15/1",
          "controllingPort": 68,
          "pins": [
            {
              "a": {
                "chip": "BC14",
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
                "chip": "BC14",
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
                "chip": "BC14",
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
                "chip": "BC14",
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
                "chip": "BC14",
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
                "chip": "BC14",
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
                "chip": "BC14",
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
                "chip": "BC14",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 7
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
                      "chip": "BC14",
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
                      "chip": "BC14",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                      "chip": "BC14",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -20,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
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
          "name": "eth1/16/1",
          "controllingPort": 72,
          "pins": [
            {
              "a": {
                "chip": "BC15",
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
                "chip": "BC15",
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
                "chip": "BC15",
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
                "chip": "BC15",
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
                "chip": "BC15",
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
                "chip": "BC15",
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
                "chip": "BC15",
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
                "chip": "BC15",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 7
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
                      "chip": "BC15",
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
                      "chip": "BC15",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                      "chip": "BC15",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
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
          "name": "eth1/17/1",
          "controllingPort": 80,
          "pins": [
            {
              "a": {
                "chip": "BC16",
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
                "chip": "BC16",
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
                "chip": "BC16",
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
                "chip": "BC16",
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
                "chip": "BC16",
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
                "chip": "BC16",
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
                "chip": "BC16",
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
                "chip": "BC16",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 7
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
                      "chip": "BC16",
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
                      "chip": "BC16",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                      "chip": "BC16",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
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
          "name": "eth1/18/1",
          "controllingPort": 84,
          "pins": [
            {
              "a": {
                "chip": "BC17",
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
                "chip": "BC17",
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
                "chip": "BC17",
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
                "chip": "BC17",
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
                "chip": "BC17",
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
                "chip": "BC17",
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
                "chip": "BC17",
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
                "chip": "BC17",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 7
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
                      "chip": "BC17",
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
                      "chip": "BC17",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                      "chip": "BC17",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
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
          "name": "eth1/19/1",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "BC18",
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
                "chip": "BC18",
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
                "chip": "BC18",
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
                "chip": "BC18",
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
                "chip": "BC18",
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
                "chip": "BC18",
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
                "chip": "BC18",
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
                "chip": "BC18",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 7
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
                      "chip": "BC18",
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
                      "chip": "BC18",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
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
          "name": "eth1/20/1",
          "controllingPort": 92,
          "pins": [
            {
              "a": {
                "chip": "BC19",
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
                "chip": "BC19",
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
                "chip": "BC19",
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
                "chip": "BC19",
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
                "chip": "BC19",
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
                "chip": "BC19",
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
                "chip": "BC19",
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
                "chip": "BC19",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/20",
                  "lane": 7
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
                      "chip": "BC19",
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
                      "chip": "BC19",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 7
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
          "name": "eth1/21/1",
          "controllingPort": 100,
          "pins": [
            {
              "a": {
                "chip": "BC20",
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
                "chip": "BC20",
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
                "chip": "BC20",
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
                "chip": "BC20",
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
                "chip": "BC20",
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
                "chip": "BC20",
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
                "chip": "BC20",
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
                "chip": "BC20",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 7
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
                      "chip": "BC20",
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
                      "chip": "BC20",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 7
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
          "name": "eth1/22/1",
          "controllingPort": 104,
          "pins": [
            {
              "a": {
                "chip": "BC21",
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
                "chip": "BC21",
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
                "chip": "BC21",
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
                "chip": "BC21",
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
                "chip": "BC21",
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
                "chip": "BC21",
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
                "chip": "BC21",
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
                "chip": "BC21",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 7
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
                      "chip": "BC21",
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
                      "chip": "BC21",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -12,
                      "pre2": 0,
                      "main": 136,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 7
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
          "name": "eth1/23/1",
          "controllingPort": 108,
          "pins": [
            {
              "a": {
                "chip": "BC22",
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
                "chip": "BC22",
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
                "chip": "BC22",
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
                "chip": "BC22",
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
                "chip": "BC22",
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
                "chip": "BC22",
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
                "chip": "BC22",
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
                "chip": "BC22",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 7
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
                      "chip": "BC22",
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
                      "chip": "BC22",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 7
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
          "name": "eth1/24/1",
          "controllingPort": 112,
          "pins": [
            {
              "a": {
                "chip": "BC23",
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
                "chip": "BC23",
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
                "chip": "BC23",
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
                "chip": "BC23",
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
                "chip": "BC23",
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
                "chip": "BC23",
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
                "chip": "BC23",
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
                "chip": "BC23",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/24",
                  "lane": 7
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
                      "chip": "BC23",
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
                      "chip": "BC23",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 7
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
          "name": "eth1/25/1",
          "controllingPort": 120,
          "pins": [
            {
              "a": {
                "chip": "BC24",
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
                "chip": "BC24",
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
                "chip": "BC24",
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
                "chip": "BC24",
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
                "chip": "BC24",
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
                "chip": "BC24",
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
                "chip": "BC24",
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
                "chip": "BC24",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 7
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
                      "chip": "BC24",
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
                      "chip": "BC24",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 7
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
          "name": "eth1/26/1",
          "controllingPort": 124,
          "pins": [
            {
              "a": {
                "chip": "BC25",
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
                "chip": "BC25",
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
                "chip": "BC25",
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
                "chip": "BC25",
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
                "chip": "BC25",
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
                "chip": "BC25",
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
                "chip": "BC25",
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
                "chip": "BC25",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 7
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
                      "chip": "BC25",
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
                      "chip": "BC25",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 7
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
          "name": "eth1/27/1",
          "controllingPort": 128,
          "pins": [
            {
              "a": {
                "chip": "BC26",
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
                "chip": "BC26",
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
                "chip": "BC26",
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
                "chip": "BC26",
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
                "chip": "BC26",
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
                "chip": "BC26",
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
                "chip": "BC26",
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
                "chip": "BC26",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 7
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
                      "chip": "BC26",
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
                      "chip": "BC26",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 136,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 7
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
          "name": "eth1/28/1",
          "controllingPort": 132,
          "pins": [
            {
              "a": {
                "chip": "BC27",
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
                "chip": "BC27",
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
                "chip": "BC27",
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
                "chip": "BC27",
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
                "chip": "BC27",
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
                "chip": "BC27",
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
                "chip": "BC27",
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
                "chip": "BC27",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/28",
                  "lane": 7
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
                      "chip": "BC27",
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
                      "chip": "BC27",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 95,
                      "post": -12,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -8,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 7
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
          "name": "eth1/29/1",
          "controllingPort": 140,
          "pins": [
            {
              "a": {
                "chip": "BC28",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
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
                  "chip": "eth1/29",
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
                  "chip": "eth1/29",
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
                  "chip": "eth1/29",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "BC28",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC28",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC28",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC28",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 7
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
                      "chip": "BC28",
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
                      "chip": "BC28",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 140,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 7
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
          "name": "eth1/30/1",
          "controllingPort": 144,
          "pins": [
            {
              "a": {
                "chip": "BC29",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 0
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
                  "chip": "eth1/30",
                  "lane": 1
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
                  "chip": "eth1/30",
                  "lane": 2
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
                  "chip": "eth1/30",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "BC29",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC29",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC29",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC29",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 7
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
                      "chip": "BC29",
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
                      "chip": "BC29",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 144,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 7
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
          "name": "eth1/31/1",
          "controllingPort": 148,
          "pins": [
            {
              "a": {
                "chip": "BC30",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
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
                  "chip": "eth1/31",
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
                  "chip": "eth1/31",
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
                  "chip": "eth1/31",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "BC30",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC30",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC30",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC30",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 7
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
                      "chip": "BC30",
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
                      "chip": "BC30",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 7
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
          "name": "eth1/32/1",
          "controllingPort": 152,
          "pins": [
            {
              "a": {
                "chip": "BC31",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 0
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
                  "chip": "eth1/32",
                  "lane": 1
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
                  "chip": "eth1/32",
                  "lane": 2
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
                  "chip": "eth1/32",
                  "lane": 3
                }
              }
            },
            {
              "a": {
                "chip": "BC31",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC31",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC31",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC31",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 7
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
                      "chip": "BC31",
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
                      "chip": "BC31",
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
          "22": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -14,
                      "pre2": 0,
                      "main": 109,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "23": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -6,
                      "pre2": 0,
                      "main": 101,
                      "post": -20,
                      "post2": 0,
                      "post3": 0
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
          "24": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -32,
                      "pre2": 0,
                      "main": 132,
                      "post": 0,
                      "post2": 0,
                      "post3": 0
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
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
          "26": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 0
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 1
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 2
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 3
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 4
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 5
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 6
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 7
                    },
                    "tx": {
                      "pre": -16,
                      "pre2": 0,
                      "main": 148,
                      "post": -4,
                      "post2": 0,
                      "post3": 0
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
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
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
          5,
          9,
          13,
          24,
          28,
          32,
          44,
          48,
          52,
          64,
          68,
          72,
          84,
          88,
          92,
          104,
          108,
          112,
          124,
          128,
          132,
          144,
          148,
          152,
          1,
          20,
          40,
          60,
          80,
          100,
          120,
          140
        ],
        "profiles": [
          12,
          11
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
              "pre": -2,
              "pre2": 0,
              "main": 117,
              "post": -8,
              "post2": 0,
              "post3": 0
            }
          }
        ]
      }
    }
  ],
  "platformSupportedProfiles": [
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
          "interfaceMode": 3,
          "interfaceType": 3
        }
      }
    },
    {
      "factor": {
        "profileID": 24
      },
      "profile": {
        "speed": 200000,
        "iphy": {
          "numLanes": 4,
          "modulation": 2,
          "fec": 11,
          "medium": 1,
          "interfaceMode": 12,
          "interfaceType": 12
        }
      }
    },
    {
      "factor": {
        "profileID": 25
      },
      "profile": {
        "speed": 200000,
        "iphy": {
          "numLanes": 4,
          "modulation": 2,
          "fec": 11,
          "medium": 3,
          "interfaceMode": 3,
          "interfaceType": 3
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
          "fec": 11,
          "medium": 3,
          "interfaceMode": 4,
          "interfaceType": 4
        }
      }
    }
  ]
}
)";
} // namespace

namespace facebook {
namespace fboss {
DarwinPlatformMapping::DarwinPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

DarwinPlatformMapping::DarwinPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace fboss
} // namespace facebook
